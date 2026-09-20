/*
 * This API is for external programs that wish to explicitly take
 * control of the multithreaded execution of FiniteFlow graphs.
 *
 * Most application do NOT need this, since FiniteFlow itself takes
 * care of spawning the threads and scheduling the evaluation of
 * FiniteFlow graphs, in reconstruction routines as well as in
 * functions that evaluate multiple points.  External programs
 * implementing their own reconstruction algorithms may, however,
 * sometimes implement their own scheduling of multi-threaded
 * evaluations.  This API enables the concurrent evaluation of
 * FiniteFlow graphs from threads spawned and scheduled by external
 * programs.
 *
 *
 * Here is the workflow to evaluate a FiniteFlow graph in a
 * multi-threaded application:
 *
 * - call ffMTEvalBegin() once, before any multi-threaded evaluation
 *
 * - evaluate the graph with ffMTEvalWithId() any number of times,
 *   even concurrently from different threads, using the thread_id
 *   argument to identify the calling thread
 *
 * - call ffMTEvalEnd() once, after all the evaluations have completed
 *
 * The ffMTEvalBegin() call specifies the number of threads n_threads
 * that will be used.  These threads will be labeled by a integer
 * id=0,...,n_threads-1.  After the call to ffThreadEvalBegin(), the
 * selected graph becomes immutable.  The returned pointer
 * FFMTGraphEvaluator* is NULL if an error occurred (e.g. if the graph
 * cannot be evaluated).  Otherwise, this pointer needs to be passed
 * as an argument to the other functions.
 *
 * Calls to ffMTEvalWithId() can then be made concurrently from
 * different threads.  This is safe, as long as concurrent calls from
 * different threads specify a distinct thread id (between 0 and
 * n_threads-1, with n_threads defined above).  Internally, the
 * thread_id is used to identify a distinct copy of the mutable
 * (per-thread) data used for the graph evaluation, thus avoiding data
 * races as long as the caller satisfies the condition above.  The
 * first time ffMTEvalWithId() is called with a given thread_id, the
 * required mutable data needed for evaluation is copied.
 *
 * The call ffMTEvalEnd() signals the end of concurrent evaluations.
 * After this, the FFMTGraphEvaluator pointer is invalidated and - if
 * possible - the graph becomes mutable again.
 */

#pragma once

#include <fflow/capi.h>
#include <fflow/ffmod.h>

#ifdef __cplusplus
extern "C" {
#endif

  typedef struct FFMTGraphEvaluator FFMTGraphEvaluator;

  FFMTGraphEvaluator * ffMTEvalBegin(FFGraph graph, unsigned n_threads);

  FFStatus ffMTEvalWithId(FFMTGraphEvaluator * thr_eval,
                          unsigned thread_id,
                          const FFUInt x[], FFMod mod,
                          FFUInt * out);

  void ffMTEvalEnd(FFMTGraphEvaluator * thr_eval);

#ifdef __cplusplus
}
#endif
