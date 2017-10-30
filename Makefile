
MPI_FLAGS=-I/usr/include/openmpi-x86_64 -pthread -Wl,-rpath -Wl,/usr/lib64/openmpi/lib -Wl,--enable-new-dtags -L/usr/lib64/openmpi/lib -lmpi_cxx -lmpi

ADIOS_13_FLAGS=-I/work/apps/adios/1.13.0-g/include -I/work/apps/dataspaces/1.6.2/include -I/work/apps/chaos/stable/include -I/work/apps/chaos/stable/include -I/work/apps/chaos/stable/include -I/work/apps/chaos/stable/include -L/work/apps/adios/1.13.0-g/lib -ladios -L/work/apps/dataspaces/1.6.2/lib -ldspaces -ldscommon -ldart -lpthread -lm -fPIC /work/apps/chaos/stable/lib/libevpath.a /work/apps/chaos/stable/lib/libffs.a /work/apps/chaos/stable/lib/libatl.a /work/apps/chaos/stable/lib/libdill.a /work/apps/chaos/stable/lib/libcercs_env.a -ldl -libverbs -fPIC -lpthread -lm

ADIOS_12_FLAGS=-I/work/apps/adios/1.12.0-g/include -I/work/apps/dataspaces/1.6.2/include -I/work/apps/chaos/stable/include -I/work/apps/chaos/stable/include -I/work/apps/chaos/stable/include -I/work/apps/chaos/stable/include -L/work/apps/adios/1.12.0-g/lib -ladios -L/work/apps/dataspaces/1.6.2/lib -ldspaces -ldscommon -ldart -lpthread -lm -fPIC /work/apps/chaos/stable/lib/libevpath.a /work/apps/chaos/stable/lib/libffs.a /work/apps/chaos/stable/lib/libatl.a /work/apps/chaos/stable/lib/libdill.a /work/apps/chaos/stable/lib/libcercs_env.a -ldl -libverbs -fPIC -lpthread -lm

ADIOS_11_FLAGS=-I/work/apps/adios/1.11.0-g/include -I/work/apps/dataspaces/1.6.2/include -I/work/apps/chaos/stable/include -I/work/apps/chaos/stable/include -I/work/apps/chaos/stable/include -I/work/apps/chaos/stable/include -I/work/apps/chaos/stable/include -I/work/apps/chaos/stable/include -I/work/apps/chaos/stable/include -I/work/apps/chaos/stable/include -I/work/apps/chaos/stable/include -I/work/apps/chaos/stable/include -L/work/apps/adios/1.11.0-g/lib -ladios -L/work/apps/dataspaces/1.6.2/lib -L/work/apps/chaos/stable/lib -L/work/apps/chaos/stable/lib -L/work/apps/chaos/stable/lib -L/work/apps/chaos/stable/lib -L/work/apps/chaos/stable/lib -ldspaces -ldscommon -ldart -levpath -lffs -latl -ldill -lcercs_env -libverbs -fPIC -lm

ADIOS_FLAGS=$(ADIOS_13_FLAGS)

LOCAL_ARRAY_API_ISSUE=-DADIOS_ISSUE_1
FLEXPATH_CASE_ISSUE=-DADIOS_ISSUE_2
FLEXPATH_BYTE_ISSUE=-DADIOS_ISSUE_3

ALL_ISSUES=$(LOCAL_ARRAY_API_ISSUE) $(FLEXPATH_CASE_ISSUE) $(FLEXPATH_BYTE_ISSUE)
FLEXPATH_ISSUES=$(FLEXPATH_CASE_ISSUE) $(FLEXPATH_BYTE_ISSUE)

errors:
	@echo "error! select one of the of the following issue targets"
	@echo "make issue_146"
	@echo "make issue_147"
	@echo "make issue_148"
	@exit

issue_146: clean
	g++ -O0 -g3 -std=c++11 put.cpp $(MPI_FLAGS) $(ADIOS_FLAGS) $(FLEXPATH_ISSUES) -o put
	g++ -O0 -g3 -std=c++11 get.cpp $(MPI_FLAGS) $(ADIOS_FLAGS) $(FLEXPATH_ISSUES) -o get

issue_147: clean
	g++ -O0 -g3 -std=c++11 put.cpp $(MPI_FLAGS) $(ADIOS_FLAGS) $(FLEXPATH_BYTE_ISSUE) -o put
	g++ -O0 -g3 -std=c++11 get.cpp $(MPI_FLAGS) $(ADIOS_FLAGS) $(FLEXPATH_BYTE_ISSUE) -o get

issue_148: clean
	g++ -O0 -g3 -std=c++11 put.cpp $(MPI_FLAGS) $(ADIOS_FLAGS) $(LOCAL_ARRAY_API_ISSUE) -o put
	g++ -O0 -g3 -std=c++11 get.cpp $(MPI_FLAGS) $(ADIOS_FLAGS) $(LOCAL_ARRAY_API_ISSUE) -o get

all: clean
	g++ -O0 -g3 -std=c++11 put.cpp $(MPI_FLAGS) $(ADIOS_FLAGS) $(ALL_ISSUES) -o put
	g++ -O0 -g3 -std=c++11 get.cpp $(MPI_FLAGS) $(ADIOS_FLAGS) $(ALL_ISSUES) -o get

none: clean
	g++ -O0 -g3 -std=c++11 put.cpp $(MPI_FLAGS) $(ADIOS_FLAGS) -o put
	g++ -O0 -g3 -std=c++11 get.cpp $(MPI_FLAGS) $(ADIOS_FLAGS) -o get


clean:
	rm -f put get conf test.bp
