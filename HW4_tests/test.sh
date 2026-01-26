#!/bin/bash

print_diffs=true
echo "PUT YOUR sim_main FILE IN THE DIRECTORY!!!"
echo
echo Running tests...
echo The test succeed if there are no diffs printed.
echo


#for filename in tests/test*.command; do
#echo Running $filename
#    test_num=`echo $filename | cut -d'.' -f1`
#    bash ${filename} > ${test_num}.YoursOut
#done
#echo

for filename in tests/examples/example*.img; do
echo Running $filename
    test_num=`echo $filename | cut -d'.' -f1`
    ./sim_main ${filename} > ${test_num}_YoursOut
done
echo

for filename in tests/segel-tests/example*.img; do
    echo Running $filename
    #var=$( cat $filename )
    #./cacheSim "../test2/example${num}_trace" ${var} > ${num}.txt
    test_num=`echo $filename | cut -d'.' -f1`
    ./sim_main ${filename} > ${test_num}_YoursOut
    #./cacheSim ${test_num}_trace ${var} > ${test_num}_YoursOut
    #diff ${test_num}_YoursOut ../test2/example${num}_output
    #echo 
done
echo
echo

#for filename in tests/test*.out; do
#    echo -n comparing $filename - 
#    test_num=`echo $filename | cut -d'.' -f1`
#    diff_result=$(diff ${test_num}.out ${test_num}.YoursOut)
#    if [ "$diff_result" != "" ]; then
#        echo The test ${test_num} didnt pass
#        echo "in: " $(cat ${test_num}.command) 
#        echo $diff_result
#    else
#        echo " SUCCESS!"
#    fi
#done
#echo

for filename in tests/examples/example*_output; do
    echo -n comparing $filename - 
    test_num=`echo $filename | cut -d'_' -f1`
    diff_result=$(diff ${test_num}_YoursOut ${test_num}_output)
    if [ "$diff_result" != "" ]; then
        echo The test ${test_num} didnt pass
        if [ "$print_diffs" = true ]; then
            echo $diff_result
        fi

    else
        echo " SUCCESS!"
    fi
done 
echo

for filename in tests/segel-tests/example*_output; do
    echo -n comparing $filename - 
    #var=$( cat $filename )
    #./cacheSim "../test2/example${num}_trace" ${var} > ${num}.txt
    test_num=`echo $filename | cut -d'_' -f1`
    diff_result=$(diff ${test_num}_YoursOut ${test_num}_output)
    if [ "$diff_result" != "" ]; then
        echo The test ${test_num} didnt pass
        if [ "$print_diffs" = true ]; then
            echo $diff_result
        fi
    else
        echo " SUCCESS!"
    fi
done
echo
echo
echo Ran all tests.
