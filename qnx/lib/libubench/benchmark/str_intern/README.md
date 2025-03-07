# str_intern <!-- omit in toc -->

This benchmark is to test and compare different implementations of string
interning, that a decision can be made which one should be included in the
`libubench` library.

The (conflicting) goals of string interning is:

- Fastest lookup time possible should a string exist
- Fastest insert time possible should a string not exist
- Smallest memory possible while parsing

The benchmark framework `str_intern` sets up a buffer of 64kB and uses low-level
Posix API to read directly from the file. This removes all memory allocations
when reading from a file. It implements tokenisation of the string itself using
simple rules (a space between any character separates to create a new token).

## 1. Implementations

| Implementation  | Details                   |
| --------------- | ------------------------- |
| `none`          | Does no string interning. |
| `forward_list`  | Use a `forward_list`.     |
| `set`           | Use a `set`.              |
| `unordered_set` | Use an `unordered_set`    |

## 2. Results

The input file is a 68M file, generated text with books from Project Gutenberg
with the input in English. The word distribution would be common English words.
While this is a good test with lots of repetition, computer generated string
interning generally has very few unique words and lots of repetitions. Thus the
search complexity will have an influence in predicting the performance in other
scenarios.

This was tested on an Intel Skylake i7-6700T on Ubuntu 22.04.5.

| Metric                      |       none | forward_list |         set | unordered_set | fixed_set_128k | fixed_set_256k | fixed_set_512k | fixed_set_1m |
| --------------------------- | ---------: | -----------: | ----------: | ------------: | -------------: | -------------: | -------------: | -----------: |
| Words                       | 11,847,228 |   11,847,228 |  11,847,228 |    11,847,228 |     11,847,228 |     11,847,228 |     11,847,228 |   11,847,228 |
| Interned Words              |          0 |      337,601 |     337,601 |       337,601 |        337,601 |        337,601 |        337,601 |      337,601 |
| Currently Allocated (bytes) |          8 |   13,748,928 |  21,851,384 |    19,258,264 |     22,899,976 |     23,948,552 |     26,045,704 |   30,240,008 |
| Max Allocated Mem (bytes)   |          8 |   13,748,928 |  21,851,475 |    19,258,339 |     22,899,976 |     23,948,552 |     26,045,704 |   30,240,008 |
| Total Allocated Mem (bytes) |          8 |   13,748,928 | 758,821,533 |   574,800,861 |     22,899,976 |     23,948,552 |     26,045,704 |   30,240,008 |
| Total Freed Mem (bytes)     |          0 |            0 | 736,970,149 |   555,542,597 |              0 |              0 |              0 |            0 |
| Allocation Count            |          1 |      350,176 |  11,876,595 |    11,876,610 |        687,779 |        687,779 |        687,779 |      687,779 |
| Free Count                  |          0 |            0 |  11,426,419 |    11,526,433 |              0 |              0 |              0 |            0 |
| Process Time (ms)           |        407 |   16,177,978 |       4,352 |         1,507 |          1,155 |            992 |            907 |          871 |
| System Time (ms)            |        464 |   16,558,119 |       4,508 |         1,591 |          1,234 |          1,059 |          1,006 |          940 |
| Elapsed Time (ms)           |        408 |   16,180,343 |       4,352 |         1,507 |          1,155 |            992 |            907 |          871 |

For QNX 7.1.0 on Raspberry Pi 4B 8GB

| Metric                      |       none | forward_list |         set | unordered_set | fixed_set_128k | fixed_set_256k | fixed_set_512k | fixed_set_1m |
| --------------------------- | ---------: | -----------: | ----------: | ------------: | -------------: | -------------: | -------------: | -----------: |
| Words                       | 11,847,228 |   11,847,228 |  11,847,228 |    11,847,228 |     11,847,228 |     11,847,228 |     11,847,228 |   11,847,228 |
| Interned Words              |          0 |      337,601 |     337,601 |       337,601 |        337,601 |        337,601 |        337,601 |      337,601 |
| Currently Allocated (bytes) |          8 |   10,840,328 |  18,942,760 |    16,833,376 |     19,991,376 |     21,039,952 |     23,137,104 |   27,331,408 |
| Max Allocated Mem (bytes)   |          8 |   10,840,328 |  18,942,848 |    16,833,448 |     19,991,376 |     21,039,952 |     23,137,104 |   27,331,408 |
| Total Allocated Mem (bytes) |          8 |   10,840,328 | 663,581,680 |   480,609,784 |     19,991,376 |     21,039,952 |     23,137,104 |   27,331,408 |
| Total Freed Mem (bytes)     |          0 |            0 | 644,638,920 |   463,776,408 |              0 |              0 |              0 |            0 |
| Allocation Count            |          1 |      338,617 |  11,850,438 |    11,850,456 |        676,220 |        676,220 |        676,220 |      676,220 |
| Free Count                  |          0 |            0 |  11,511,821 |    11,511,838 |              0 |              0 |              0 |            0 |
| Process Time (ms)           |        645 |   39,208,790 |      13,265 |         7,958 |          3,786 |          3,222 |          2,941 |        2,809 |
| System Time (ms)            |        680 |   39,219,395 |      13,394 |         8,081 |          3,910 |          3,345 |          3,060 |        2,926 |
| Elapsed Time (ms)           |        680 |   39,209,008 |      13,390 |         8,076 |          3,909 |          3,343 |          3,058 |        2,925 |

For QNX 8.0.0 on Raspberry Pi 4B 8GB

| Metric                      |       none | forward_list |         set | unordered_set | fixed_set_128k | fixed_set_256k | fixed_set_512k | fixed_set_1m |
| --------------------------- | ---------: | -----------: | ----------: | ------------: | -------------: | -------------: | -------------: | -----------: |
| Words                       | 11,847,228 |   11,847,228 |  11,847,228 |    11,847,228 |     11,847,228 |     11,847,228 |     11,847,228 |   11,847,228 |
| Interned Words              |          0 |      337,601 |     337,601 |       337,601 |        337,601 |        337,601 |        337,601 |      337,601 |
| Currently Allocated (bytes) |          8 |   10,837,078 |  18,939,510 |    16,830,126 |     19,988,126 |     21,036,702 |     23,133,854 |   27,328,158 |
| Max Allocated Mem (bytes)   |          8 |   10,837,078 |  18,939,598 |    16,830,198 |     19,988,126 |     21,036,702 |     23,133,854 |   27,328,158 |
| Total Allocated Mem (bytes) |          8 |   10,837,078 | 663,568,981 |   480,597,085 |     19,988,126 |     21,036,702 |     23,133,854 |   27,328,158 |
| Total Freed Mem (bytes)     |          0 |            0 | 644,629,471 |   463,766,959 |              0 |              0 |              0 |            0 |
| Allocation Count            |          1 |      338,617 |  11,850,438 |    11,850,456 |        676,220 |        676,220 |        676,220 |      676,220 |
| Free Count                  |          0 |            0 |  11,511,821 |    11,511,838 |              0 |              0 |              0 |            0 |
| Process Time (ms)           |        675 |   36,146,387 |      12,240 |         6,293 |          3,844 |          3,241 |          2,943 |        2,755 |
| System Time (ms)            |        784 |   36,291,319 |      12,459 |         6,482 |          4,038 |          3,428 |          3,132 |        2,941 |
| Elapsed Time (ms)           |        774 |   36,181,673 |      12,406 |         6,448 |          4,006 |          3,401 |          3,103 |        2,916 |

For NetBSD 10.1 on Raspberry Pi 4B 8GB

| Metric                      |       none | forward_list |         set | unordered_set | fixed_set_128k | fixed_set_256k | fixed_set_512k | fixed_set_1m |
| --------------------------- | ---------: | -----------: | ----------: | ------------: | -------------: | -------------: | -------------: | -----------: |
| Words                       | 11,847,228 |              |  11,847,228 |    11,847,228 |     11,847,228 |     11,847,228 |     11,847,228 |   11,847,228 |
| Interned Words              |          0 |              |     337,601 |       337,601 |        337,601 |        337,601 |        337,601 |      337,601 |
| Currently Allocated (bytes) |          8 |              |  21,851,384 |    19,258,264 |     22,899,976 |     23,948,552 |     26,045,704 |   30,240,008 |
| Max Allocated Mem (bytes)   |          8 |              |  21,851,475 |    19,258,339 |     22,899,976 |     23,948,552 |     26,045,704 |   30,240,008 |
| Total Allocated Mem (bytes) |          8 |              | 758,821,533 |   574,800,861 |     22,899,976 |     23,948,552 |     26,045,704 |   30,240,008 |
| Total Freed Mem (bytes)     |          0 |              | 736,970,149 |   555,542,597 |              0 |              0 |              0 |            0 |
| Allocation Count            |          1 |              |  11,876,595 |    11,876,610 |        687,779 |        687,779 |        687,779 |      687,779 |
| Free Count                  |          0 |              |  11,526,419 |    11,526,433 |              0 |              0 |              0 |            0 |
| Process Time (ms)           |      3,240 |              |      35,258 |        11,350 |          9,751 |          8,784 |          8,137 |        7,832 |
| System Time (ms)            |      3,233 |              |      35,208 |        11,347 |          9,761 |          8,790 |          8,131 |        7,813 |
| Elapsed Time (ms)           |      3,240 |              |      35,262 |        11,351 |          9,752 |          8,785 |          8,137 |        7,833 |

### 2.1. None

The base line. No words are interned, the file is read. On the machine (using
68MB file, 32GB RAM in the system, reading from an external HDD).

### 2.2. Forward List

A simple, forward search every time a word is added.

- Has very very memory allocations and the memory representation is reasonably
  efficient.
- The time is extremely slow and is only useful for a small set of data.

Complexity analysis:

- Search is O(n).
- Insert is O(1).
- Interning a file is then O(n^2 + m), where m is the number of unique words.

### 2.3. Set

A simple set. The `string_view` is provided and stored as a `string`.

- Approximately one allocation / free per string being interned. The
  implementation appears to convert the `string_view` to a `string`, even if it
  isn't added.
- From 68MB, approximately 20.8MB words are unique over 337,601 words.

Complexity analysis:

- Search is O(1)
- Insert is O(log(n))
- Interning a file is then O(n + m.log(m)) where m is the number of unique
  words.
- See [Stack
  Overflow](https://stackoverflow.com/questions/2558153/what-is-the-underlying-data-structure-of-a-stl-set-in-c/51944661#51944661)

### 2.4. Unordered Set

Same as using a set, but the structure is stored in an
`unordered_set<std::string>`.

- Allocation count is about the same as a `set`.
- Much faster by 4x, and slightly less memory usage.

- Search is O(1)
- Insert is O(1). This ignores the resizing of the array periodically as the set
  grows.
- Interning a file is then O(n + m) where m is the number of unique words.
- See [Stack
  Overflow](https://stackoverflow.com/questions/2558153/what-is-the-underlying-data-structure-of-a-stl-set-in-c/51944661#51944661)

### 2.5. Fixed Set

This is a custom implementation of a set using a fixed number of buckets. The
value is the number of buckets.

- Search is O(1) indexed into a vector (and then for collisions, it's a linear
  search in a `forward_list`).
- Insert is O(1), inserting the string in a fixed location at the beginning of a
  `forward_list` and adding to the list int he bucket.

The time decreases as the buckets increase as there are fewer collisions.
