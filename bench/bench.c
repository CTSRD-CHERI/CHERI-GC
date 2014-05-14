// This is adapted from a benchmark written by John Ellis and Pete Kovac
// of Post Communications.
// It was modified by Hans Boehm of Silicon Graphics.
// Translated to C++ 30 May 1997 by William D Clinger of Northeastern Univ.
// Translated to C 15 March 2000 by Hans Boehm, now at HP Labs.
//
//      This is no substitute for real applications.  No actual application
//      is likely to behave in exactly this way.  However, this benchmark was
//      designed to be more representative of real applications than other
//      Java GC benchmarks of which we are aware.
//      It attempts to model those properties of allocation requests that
//      are important to current GC techniques.
//      It is designed to be used either to obtain a single overall performance
//      number, or to give a more detailed estimate of how collector
//      performance varies with object lifetimes.  It prints the time
//      required to allocate and collect balanced binary trees of various
//      sizes.  Smaller trees result in shorter object lifetimes.  Each cycle
//      allocates roughly the same amount of memory.
//      Two data structures are kept around during the entire process, so
//      that the measured performance is representative of applications
//      that maintain some live in-memory data.  One of these is a tree
//      containing many pointers.  The other is a large array containing
//      double precision floating point numbers.  Both should be of comparable
//      size.
//
//      The results are only really meaningful together with a specification
//      of how much memory was used.  It is possible to trade memory for
//      better time performance.  This benchmark should be run in a 32 MB
//      heap, though we don't currently know how to enforce that uniformly.
//
//      Unlike the original Ellis and Kovac benchmark, we do not attempt
//      measure pause times.  This facility should eventually be added back
//      in.  There are several reasons for omitting it for now.  The original
//      implementation depended on assumptions about the thread scheduler
//      that don't hold uniformly.  The results really measure both the
//      scheduler and GC.  Pause time measurements tend to not fit well with
//      current benchmark suites.  As far as we know, none of the current
//      commercial Java implementations seriously attempt to minimize GC pause
//      times.

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#ifdef GC
#  include "gc.h"
#define tf_cap_t
#define tf_ptr_valid(x) ((x)!=NULL)
#define tf_invalid_ptr() 0
#define tf_store_cap(x,y) ((x)=(y))

void __LOCK_MALLOC ()
{
}
void __UNLOCK_MALLOC ()
{
}
#endif

#ifdef GC_NOCAP
#define GC_bench_malloc malloc
#define GC_bench_calloc calloc
#define GC_bench_free   free
#define tf_cap_t
#define tf_ptr_valid(x) ((x)!=NULL)
#define tf_invalid_ptr() 0
#define tf_store_cap(x,y) ((x)=(y))
#endif // GC_NOCAP

#ifdef GC_CHERI
#include <gc.h>
#define GC_bench_malloc GC_malloc
#define GC_bench_calloc tf_cheri_calloc
#define GC_bench_free   tf_cheri_free
#define tf_cap_t __capability
#define tf_invalid_ptr GC_INVALID_PTR
#define tf_ptr_valid GC_PTR_VALID
#define tf_store_cap GC_STORE_CAP

/*void
do_nothing_with (tf_cap_t void * p)
{
  volatile tf_cap_t * q = p;
  printf("p is 0x%llx\n", (GC_ULL) GC_cheri_getbase(p));
}*/

tf_cap_t void *
tf_cheri_calloc (size_t num, size_t sz)
{
  tf_cap_t void * p = GC_malloc(num*sz);
  GC_cap_memset(p, 0);
  return p;
}

void
tf_cheri_free (tf_cap_t void * ptr)
{
}
#endif // GC_CHERI

#ifdef GC_NONE
#include <machine/cheri.h>
#include <machine/cheric.h>
#define tf_cheri_ptr     cheri_ptr
#define tf_cheri_getbase cheri_getbase
#define GC_bench_malloc tf_no_gc_malloc
#define GC_bench_calloc tf_no_gc_calloc
#define GC_bench_free   tf_no_gc_free
#define tf_cap_t __capability
#define tf_invalid_ptr() NULL
#define tf_ptr_valid(x) ((x)!=NULL)
#define tf_store_cap(x,y) ((x)=(y))

#include <stdlib.h>
tf_cap_t void *
tf_no_gc_malloc (size_t sz)
{
  void * p = malloc(sz);
  if (p)
    return tf_cheri_ptr(p, sz);
  else
    return tf_invalid_ptr();
}
tf_cap_t void *
tf_no_gc_calloc (size_t num, size_t sz)
{
  void * p = calloc(num, sz);
  if (p)
    return tf_cheri_ptr(p, num*sz);
  else
    return tf_invalid_ptr();
}
void
tf_no_gc_free (tf_cap_t void * ptr)
{
  free(tf_cheri_getbase(ptr));
}
#endif // GC_NONE

#ifdef PROFIL
  extern void init_profiling();
  extern dump_profile();
#endif

//  These macros were a quick hack for the Macintosh.
//
//  #define currentTime() clock()
//  #define elapsedTime(x) ((1000*(x))/CLOCKS_PER_SEC)

#define currentTime() stats_rtclock()
#define elapsedTime(x) (x)

/* Get the current time in milliseconds */

unsigned
stats_rtclock( void )
{
  struct timeval t;
  struct timezone tz;

  if (gettimeofday( &t, &tz ) == -1)
    return 0;
  return (t.tv_sec * 1000 + t.tv_usec / 1000);
}

static const int kStretchTreeDepth    = 18;      // about 16Mb
static const int kLongLivedTreeDepth  = 16;  // about 4Mb
static const int kArraySize  = 500000;  // about 4Mb
static const int kMinTreeDepth = 4;
static const int kMaxTreeDepth = 16;

typedef struct Node0_struct {
        tf_cap_t struct Node0_struct * left;
        tf_cap_t struct Node0_struct * right;
        int i, j;
  //int pad0,pad1,pad2,pad3,pad4,pad5,pad6,pad7,pad8,pad9;
} Node0;

#ifdef HOLES
#   define HOLE() GC_NEW(Node0);
#else
#   define HOLE()
#endif

typedef tf_cap_t Node0 *Node;

void init_Node(Node me, Node l, Node r) {
    tf_store_cap(me -> left, l);
    tf_store_cap(me -> right, r);
}

#ifndef GC
#ifdef GC_CHERI
  #define destroy_Node(x)
#else
  void destroy_Node(Node me) {
    if (tf_ptr_valid(me -> left)) {
	destroy_Node(me -> left);
    }
    if (tf_ptr_valid(me -> right)) {
	destroy_Node(me -> right);
    }
    GC_bench_free(me);
  }
#endif
#endif

// Nodes used by a tree of a given size
static int TreeSize(int i) {
        return ((1 << (i + 1)) - 1);
}

// Number of iterations to use for a given tree depth
static int NumIters(int i) {
        return 2 * TreeSize(kStretchTreeDepth) / TreeSize(i);
}

// Build tree top down, assigning to older objects.
static void Populate(int iDepth, Node thisNode) {
        if (iDepth<=0) {
                return;
        } else {
                iDepth--;
#		ifdef GC
                  thisNode->left  = GC_NEW(Node0); HOLE();
                  thisNode->right = GC_NEW(Node0); HOLE();
#		else
                  tf_store_cap(thisNode->left, GC_bench_calloc(1, sizeof(Node0)));
                  tf_store_cap(thisNode->right, GC_bench_calloc(1, sizeof(Node0)));
#		endif
                Populate (iDepth, thisNode->left);
                Populate (iDepth, thisNode->right);
        }
}

// Build tree bottom-up
static Node MakeTree(int iDepth) {
	Node result;
        if (iDepth<=0) {
#	    ifndef GC
		tf_store_cap(result, GC_bench_calloc(1, sizeof(Node0)));
#	    else
		result = GC_NEW(Node0); HOLE();
#	    endif
	    /* result is implicitly initialized in both cases. */
	    return result;
        } else {
	    Node left;
      tf_store_cap(left, MakeTree(iDepth-1));
	    Node right;
      tf_store_cap(right, MakeTree(iDepth-1));
#	    ifndef GC
		tf_store_cap(result, GC_bench_malloc(sizeof(Node0)));
#	    else
		result = GC_NEW(Node0); HOLE();
#	    endif
	    init_Node(result, left, right);
	    return result;
        }
}

static void PrintDiagnostics() {
#if 0
        long lFreeMemory = Runtime.getRuntime().freeMemory();
        long lTotalMemory = Runtime.getRuntime().totalMemory();

        System.out.print(" Total memory available="
                         + lTotalMemory + " bytes");
        System.out.println("  Free memory=" + lFreeMemory + " bytes");
#endif
}

static void TimeConstruction(int depth) {
        long    tStart, tFinish;
        int     iNumIters = NumIters(depth);
        Node    tempTree;
	int 	i;

	printf("Creating %d trees of depth %d\n", iNumIters, depth);
        
        tStart = currentTime();
        for (i = 0; i < iNumIters; ++i) {
#		ifndef GC
                  tf_store_cap(tempTree, GC_bench_calloc(1, sizeof(Node0)));
#		else
                  tempTree = GC_NEW(Node0);
#		endif
                Populate(depth, tempTree);
#		ifndef GC
                  destroy_Node(tempTree);
#		endif
                tempTree = tf_invalid_ptr();
        }
        tFinish = currentTime();
        printf("\tTop down construction took %d msec\n",
               (int) elapsedTime(tFinish - tStart));
             
        tStart = currentTime();
        for (i = 0; i < iNumIters; ++i) {
                tempTree = MakeTree(depth);
#		ifndef GC
                  destroy_Node(tempTree);
#		endif
                tempTree = tf_invalid_ptr();
        }
        tFinish = currentTime();
        printf("\tBottom up construction took %d msec\n",
               (int) elapsedTime(tFinish - tStart));

}

int main() {
        Node    root;
        Node    longLivedTree;
        Node    tempTree;
        long    tStart, tFinish;
        long    tElapsed;
  	int	i, d;
	tf_cap_t double 	*array;


  setbuf(stdout, NULL);

#ifdef GC
  printf("The `GC' macro is defined (i.e. Boehm)\n");
 // GC_full_freq = 30;
 // GC_free_space_divisor = 16;
 // GC_enable_incremental();
#endif
#ifndef GC
  printf("The `GC' macro is NOT defined\n");
#if defined(GC_NOCAP)
  printf("GC_NOCAP is defined\n");
#elif defined(GC_NONE)
  printf("GC_NONE is defined\n");
#elif defined(GC_CHERI)
  printf("GC_CHERI is defined\n");
#else
  #error "Define one of GC, GC_NOCAP, GC_NONE, GC_CHERI"
#endif
#endif

#ifdef GC_CHERI
    GC_debug_dump();
#endif

  printf("sizeof Node0: %d\n", (int) sizeof(Node0));
  
	printf("Garbage Collector Test\n");
 	printf(" Live storage will peak at %d bytes.\n\n",
               (int) (2 * sizeof(Node0) * TreeSize(kLongLivedTreeDepth) +
               sizeof(double) * kArraySize));
        printf(" Stretching memory with a binary tree of depth %d\n",
               kStretchTreeDepth);
        PrintDiagnostics();
#	ifdef PROFIL
	    init_profiling();
#	endif
       
        tStart = currentTime();
        
        // Stretch the memory space quickly
        tf_store_cap(tempTree, MakeTree(kStretchTreeDepth));
        //printf("Not actually doing that\n");

#	ifndef GC
          destroy_Node(tempTree);
#	endif
        tempTree = tf_invalid_ptr();

        // Create a long lived object
        printf(" Creating a long-lived binary tree of depth %d\n",
               kLongLivedTreeDepth);
#	ifndef GC
          tf_store_cap(longLivedTree, GC_bench_calloc(1, sizeof(Node0)));
#	else 
          longLivedTree = GC_NEW(Node0);
#	endif
        Populate(kLongLivedTreeDepth, longLivedTree);

#ifdef GC_CHERI
  GC_debug_dump();
#endif // GC_CHERI

        // Create long-lived array, filling half of it
	printf(" Creating a long-lived array of %d doubles\n", kArraySize);
#	ifndef GC
          //tf_store_cap(array, GC_bench_malloc(kArraySize * sizeof(double)));
          printf("Not actually doing that, either\n");
#	else
#	  ifndef NO_PTRFREE
            //array = GC_MALLOC_ATOMIC(sizeof(double) * kArraySize);
#	  else
            //array = GC_MALLOC(sizeof(double) * kArraySize);
#	  endif
#	endif
          printf("Not populating array\n");
        for (i = 0; i < kArraySize/2; ++i) {
                //array[i] = 1.0/i;
        }
        PrintDiagnostics();

        for (d = kMinTreeDepth; d <= kMaxTreeDepth; d += 2) {
                TimeConstruction(d);
        }

        if (!tf_ptr_valid(longLivedTree) || array[1000] != 1.0/1000)
		fprintf(stderr, "Failed\n");
                                // fake reference to LongLivedTree
                                // and array
                                // to keep them from being optimized away

        tFinish = currentTime();
        tElapsed = elapsedTime(tFinish-tStart);
        PrintDiagnostics();
        printf("Completed in %d msec\n", (int) tElapsed);
#	ifdef GC
	  printf("Completed %d collections\n", (int) GC_gc_no);
	  printf("Heap size is %d\n", (int) GC_get_heap_size());
#       endif
#	ifdef PROFIL
	  dump_profile();
#	endif
#ifdef GC_CHERI
    GC_debug_dump();
#endif
}
