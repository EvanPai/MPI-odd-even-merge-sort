CXX=mpicc
exe=odd_even_sort
obj=odd_even_sort.c

$(exe): $(obj)
	$(CXX) -o $(exe) $(obj)

.PHONY: clean
clean:
	rm $(exe) 