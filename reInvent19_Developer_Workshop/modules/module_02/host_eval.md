# Part 1: Experience FPGA Acceleration

## Algorithm Overview  

The application used in this lab reads a stream of incoming documents, and computes a score for each document based on the user’s interest, represented by a search array. It is representative of real-time filtering systems that monitor news feeds and send relevant articles to end users.

In practical scenarios, the number and size of the documents to be searched can be very large and because the monitoring of events must run in real time, a smaller execution time is required for processing all the documents.

The core of the application is a Bloom filter, a space-efficient probabilistic data structure used to test whether an element is a member of a set. The algorithm attempts to find the best matching documents for a specific search array. The search array is the filter that matches documents against the user’s interest. In this application, each document is reduced to a set of 32-bit words, where each word is a pair of 24-bit word ID and 8-bit frequency representing the occurrence of the word ID in the document. The search array consists of a smaller set of word IDs and each word ID has a weight associated with it, which represents the significance of the word. The application computes a score for each document to determine its relevance to the given search array.

The algorithm can be divided in two sections:  
* Computing the hash function of the words and creating output flags
* Computing document scores based on the previously computed output flags

## Run the Application on the CPU

1. Navigate to the `cpu_src` directory and run the following command.

    ```bash 
    cd ~/SDAccel-AWS-F1-Developer-Labs/modules/module_02/cpu_src
    make run
    ```

2. The output is as follows.
    ```
    Total execution time of CPU                        |  4112.5895 ms
    Compute Hash & Output Flags processing time        |  3660.4433 ms
    Compute Score processing time                      |   452.1462 ms
    --------------------------------------------------------------------
    ```

>**NOTE:** The performance number might vary depending on the EC2 instance type and workload activity at that time.

The above command computes the score for 100,000 documents, amounting to 1.39 GBytes of data. The execution time is 4.112 seconds and throughput is computed as follows:

Throughput = Total data/Total time = 1.39 GB/4.112s = 338 MB/s

3. It is estimated that in 2012, all the data in the American Library of Congress amounted to 15 TB. Based on the above numbers, we can estimate that run processing the entire American Library of Congress on the host CPU would take about 12.3 hours (15TB / 338MB/s).

## Profiling the Application

To improve the performance, you need to identify bottlenecks where the application is spending the majority of the total running time.

As we can see from the execution times in the previous step, the applications spends 89% of time in calculating the hash function and 11 % of the time in computing the document score.

## Identify Functions to Accelerate on FPGA

The algorithm can be divided into two sections
* Computing output flags created from the hash function of every word in all documents
* Computing document score based on output flags created above
   
Let's evaluate which of these sections are a good fit for FPGA.

### Evaluating the Hash Function 

1. Open `MurmurHash2.c` file with a file editor.

2. The `MurmurHash2` hash function code is as follows:

```cpp
unsigned int MurmurHash2 ( const void * key, int len, unsigned int seed )
{
  // 'm' and 'r' are mixing constants generated offline.
  // They're not really 'magic', they just happen to work well.

  const unsigned int m = 0x5bd1e995;
  //	const int r = 24;

  // Initialize the hash to a 'random' value
  unsigned int h = seed ^ len;

  // Mix 4 bytes at a time into the hash
  const unsigned char * data = (const unsigned char *)key;

  switch(len)
  {
    case 3: h ^= data[2] << 16;
    case 2: h ^= data[1] << 8;
    case 1: h ^= data[0];
    h *= m;
  };

  // Do a few final mixes of the hash to ensure the last few
  // bytes are well-incorporated.
  h ^= h >> 13;
  h *= m;
  h ^= h >> 15;
  
  return h;
}   
```

* From the above code, you can see that the function reads a word from the memory and calculates hash output.

* The compute of hash for a single word ID consists of four XOR, 3 arithmetic shifts, and two multiplication operations.

* A shift of 1-bit in an arithmetic shift operation takes one clock cycle on the CPU.

* The three arithmetic operations shift a total of 44-bits (when`len=3` in the above code) to compute the hash which requires 44 clock cycles just to shift the bits on the host CPU. On the FPGA, it is possible to create custom architectures and therefore create an accelerator that will shift data by an arbitrary number of bits in a single clock cycle.

* The FPGA also has dedicated DSP units, which perform multiplications faster than the CPU. Even though the CPU runs at a frequency 8 times higher than the FPGA, the arithmetic shift and multiplication operations can perform faster on the FPGA because of its customizable hardware architecture.

* Therefore this function is a good candidate for FPGA acceleration.

3. Close the file.

### Evaluating the "Compute Output Flags from Hash" code section

1. Open `compute_score_host.cpp` file in the file editor. 

2. The code at lines 32-58 which computes output flags is shown below.

```cpp
// Compute output flags based on hash function output for the words in all documents
for(unsigned int doc=0;doc<total_num_docs;doc++) 
{
  profile_score[doc] = 0.0;
  unsigned int size = doc_sizes[doc];
  
  for (unsigned i = 0; i < size ; i++)
  { 
    unsigned curr_entry = input_doc_words[size_offset+i];
    unsigned word_id = curr_entry >> 8;
    unsigned hash_pu =  MurmurHash2( &word_id , 3,1);
    unsigned hash_lu =  MurmurHash2( &word_id , 3,5);
    bool doc_end = (word_id==docTag);
    unsigned hash1 = hash_pu&hash_bloom;
    bool inh1 = (!doc_end) && (bloom_filter[ hash1 >> 5 ] & ( 1 << (hash1 & 0x1f)));
    unsigned hash2 = (hash_pu+hash_lu)&hash_bloom;
    bool inh2 = (!doc_end) && (bloom_filter[ hash2 >> 5 ] & ( 1 << (hash2 & 0x1f)));
    
    if (inh1 && inh2) {
      inh_flags[size_offset+i]=1;
    } else {
      inh_flags[size_offset+i]=0;
    }
  }
  
  size_offset+=size;
}

```

* From the above code, we see that we are computing two hash outputs for each word in all the documents and creating output flags accordingly.

* We already determined that the Hash function(MurmurHash2()) is a good candidate for acceleration on FPGA.

* Computation of the hash (`MurmurHash2()`) of one word is independent of other words and can be done in parallel thereby improving the execution time.

* The algorithm makes sequential access to the `input_doc_words` array. This is an important property as it allows very efficient accesses to DDR when implemented in the FPGA.  

 4. Close the file. 
 
This code section is a a good candidate for FPGA as the hash function can run faster on FPGA and we can compute hashes for multiple words in parallel by reading multiple words from DDR in burst mode. 


### Evaluating the "Compute Document Score" code section

The code for computing the document score is shown below:

```cpp
for(unsigned int doc=0, n=0; doc<total_num_docs;doc++)
{
  profile_score[doc] = 0.0;
  unsigned int size = doc_sizes[doc];

  for (unsigned i = 0; i < size ; i++,n++)
  {
    if (inh_flags[n])
    {
      unsigned curr_entry = input_doc_words[n];
      unsigned frequency = curr_entry & 0x00ff;
      unsigned word_id = curr_entry >> 8;
      profile_score[doc]+= profile_weights[word_id] * (unsigned long)frequency;
    }
  }
}
```

* You can see that the compute score requires one memory access to `profile_weights`, one accumulation and one multiplication operation.

* The memory accesses are random, since they depend on the word ID and therefore the content of each document. 

* The size of the `profile_weights` array is 128 MB and has to be stored in DDR memory connected to the FPGA. Non-sequential accesses to DDR are big performance bottlenecks. Since accesses to the `profile_weights` array are random, implementing this function on the FPGA wouldn't provide much performance benefit, And since this function takes only about 11% of the total running time, we can keep this function on the host CPU. 

Based on this analysis, it is only beneficial to accelerate the "Compute Output Flags from Hash" section on the FPGA. Execution of the "Compute Document Score" section can be kept on the host CPU.


## Run the Application on the FPGA

For the purposes of this lab, we have implemented the FPGA accelerator with an 8x parallelization factor. It processes 8 input words in parallel, producing 8 output flags in parallel each clock cycle. Each output flag is stored as byte and indicates whether the corresponding word is present in the search array. Since each word requires two calls to the MurmurHash2 function, this means that the accelerator performs 16 hash computations in parallel. In addition, we have optimized the host application to efficiently interact with the parallelized FPGA-accelerator. The result is an application which runs significantly faster, thanks to FPGAs and AWS F1 instances.

1. Run the following make command for running optimized application on FPGA

   ```bash
   make run_fpga
   ```

2. The output is as follows:

   ```
   --------------------------------------------------------------------
    Executed FPGA accelerated version  |   552.5344 ms   ( FPGA 528.744 ms )
    Executed Software-Only version     |   3864.4070 ms
   --------------------------------------------------------------------
    Verification: PASS
   ```

Throughput = Total data/Total time = 1.39 GB/552.534ms = 2.516 GB/s

You can see that by efficiently leveraging FPGA acceleration, the throughput of the application has increased by a factor of 7.  

3. With FPGA acceleration, processing the entire American Library of Congress would take about 1.65 hours (15TB/2.52GB/s), as opposed to 12.3 hours with a software-only approach.

## Conclusion

In this lab, you have seen how to profile an application and determine which parts are best suited for FPGA acceleration. You've also experienced that once an accelerator is efficiently implemented, FPGA-accelerated applications on AWS F1 instances execute significantly faster than conventional software-only applications.

In the next lab you will dive deeper into the details of the FPGA-accelerated application and learn some of the fundamental optimization techniques leveraged in this example. In particular, you will discover how to optimize data movements between host and FPGA, how to efficiently invoke the FPGA kernel and how to overlap computation on the host CPU and the FPGA to maximize application performance.

---------------------------------------

<p align="center"><b>
Start the next step: <a href="./data_movement.md">2: Optimizing CPU and FPGA interactions</a>
</b></p>
