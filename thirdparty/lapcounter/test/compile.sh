gcc ../src/lapcounter.c \
    ../src/utils/vector/vector.c \
    ../src/utils/vector/point/point.c \
    test.c \
    -lm \
    -o test.out
gcc createCanDump.c -o createCanDump.out
