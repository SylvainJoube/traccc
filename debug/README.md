# Is the implementation of SparseCCl broken ?

## Simple recursive implementation

So far, I don't see any errors, and I'll be very interested in having more data to do further testing. I also made a very simple data generator (random pixels generation with different sparsities).

I made a simple cluster count verification by implementing a simple naive recursive CCL algorithm (ok for the usual module size 336 x 1280, but starts to take quite a bit of ram for an 65536 x 65536 image, like 20GB).
- On the csv data from the tml_detector (10 events) everything seems to work properly
- On some randomly generated data with different sparsity values, still works fine
- On some data generated with Stephen's python file, I also got the expected 1500 cluster count (with SparseCCL and my recursive implementation)

So to me, everything seems to be fine, but I may be wrong of course and have missed something.

Here is my output :

```bash
$ ./bin/sparse_ccl_debug tml_detector/trackml-detector.csv tml_pixels 10 /home/sylvain/Desktop/traccc-local/traccc/data//ccl_test_gen_data.csv

vvvvvv BEGIN TESTS STEPHEN DATA vvvvvv
WARNING : THIS MAY TAKE A LOT OF RAM. LIKE 20Go.

data path = /home/sylvain/Desktop/traccc-local/traccc/data/ccl_test_gen_data.csv

Processing Stephen file 1 / 1...
Processing event file 1 / 1...
100% 
- errors : 0

==> Statistics ... 
- 1 modules tested
- 0 error on cluster count
- 1500 clusters found
- 9788 total pixels read

^^^^^^ END TESTS STEPHEN CSV DATA ^^^^^^

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