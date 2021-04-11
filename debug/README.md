# Is the implementation of SparseCCl broken ?

## Simple recursive implementation

I (Sylvain Joube) made a small test to check wether or not I had the same number of clusters with a simple recursive implementation od CCL.

For now, I don't see any errors, and I'll be very interested in having more data to do further testing. I also made a very simple data generator (random pixels generation with different sparsities).

Here is my output :

```bash
$ ./bin/sparse_ccl_debug tml_detector/trackml-detector.csv tml_pixels 10
Running ./sparse_ccl_debug tml_detector/trackml-detector.csv tml_pixels 10
vvvvvv BEGIN TESTS ON CSV DATA vvvvvv

Processing event file 1 / 10...
00% 10% 20% 30% 40% 50% 60% 70% 80% 90% 100% 
- errors : 0

Processing event file 2 / 10...
00% 10% 20% 30% 40% 50% 60% 70% 80% 90% 100% 
- errors : 0

Processing event file 3 / 10...
00% 10% 20% 30% 40% 50% 60% 70% 80% 90% 100% 
- errors : 0

Processing event file 4 / 10...
00% 10% 20% 30% 40% 50% 60% 70% 80% 90% 100% 
- errors : 0

Processing event file 5 / 10...
00% 10% 20% 30% 40% 50% 60% 70% 80% 90% 100% 
- errors : 0

Processing event file 6 / 10...
00% 10% 20% 30% 40% 50% 60% 70% 80% 90% 100% 
- errors : 0

Processing event file 7 / 10...
00% 10% 20% 30% 40% 50% 60% 70% 80% 90% 100% 
- errors : 0

Processing event file 8 / 10...
00% 10% 20% 30% 40% 50% 60% 70% 80% 90% 100% 
- errors : 0

Processing event file 9 / 10...
00% 10% 20% 30% 40% 50% 60% 70% 80% 90% 100% 
- errors : 0

Processing event file 10 / 10...
00% 10% 20% 30% 40% 50% 60% 70% 80% 90% 100% 
- errors : 0


==> Statistics ... 
- 38784 modules tested
- 0 error on cluster count
- 380554 clusters found
- 2041344 total pixels read

^^^^^^ END TESTS ON CSV DATA ^^^^^^

vvvvvv BEGIN TESTS ON GENERATED DATA vvvvvv

checking sparsity 0.0001% ...
checking sparsity 0.001% ...
checking sparsity 0.01% ...
checking sparsity 0.1% ...
checking sparsity 1% ...
checking sparsity 10% ...

Terminated with 0 errors.

^^^^^^ END TESTS ON GENERATED DATA ^^^^^^
```