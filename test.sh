./sim 16 4 0 0 0 0 0 traces/val_gcc_trace_mem.txt > out.txt
diff -iw out.txt validation\ runs/val_1.txt
./sim 32 16 0 0 0 0 0 traces/val_perl_trace_mem.txt > out.txt
diff -iw out.txt validation\ runs/val_2.txt
./sim 16 4 32 2048 8 0 0 traces/val_gcc_trace_mem.txt > out.txt
diff -iw out.txt validation\ runs/val_extra_1.txt
./sim 32 8 32 1024 4 2048 8 traces/val_perl_trace_mem.txt > out.txt
diff -iw out.txt validation\ runs/val_extra_2.txt