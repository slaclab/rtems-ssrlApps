#define __RTEMS_VIOLATE_KERNEL_VISIBILITY__
#include <rtems.h>
#include <rtems/libcsupport.h>
//#include <rtems/score/cpu.h>
//#include <rtems/score/thread.h>
#include <stdio.h>

int
memUsageDump(int doit)
{
Heap_Information_block info;
int	rval, heapsz;

	fprintf(stderr,"WARNING: 'memUsageDump' temporarily disables thread dispatching\n");
	fprintf(stderr,"         thus killing real-time preformance! Use for diagnostics only...\n");

	if ( !doit ) {
		fprintf(stderr,"...bailing out. Call with a nonzero argument to proceed\n");
		return -1;
	} 

	_Thread_Disable_dispatch();
	rval = (HEAP_GET_INFORMATION_SUCCESSFUL !=
			_Heap_Get_information( &_Workspace_Area, &info));
	_Thread_Enable_dispatch();

	if ( rval ) {
		fprintf(stderr,"ERROR: unable to retrieve RTEMS workspace info\n");
	} else {
		printf("Workspace usage: free %ib, used %ib, configured size %ib\n",
				info.free_size, info.used_size, _Configuration_Table->work_space_size);
	}

	heapsz = malloc_free_space();

	if ( -1 == heapsz ) {
		fprintf(stderr,"ERROR: unable to retrieve malloc heap info\n");
		rval = -1;
	} else {
		printf("Free space on the malloc heap: %ib\n",heapsz);
	}

	/* The malloc heap is semaphore protected */
	return rval;
}
