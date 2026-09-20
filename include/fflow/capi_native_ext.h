/*
 * This is a C API for extending finiteflow with "native" algorithms.
 *
 * A native algorithm defines a new type of node performing a
 * numerical evaluation, implemented directly in C or any other
 * language that can interface with C.  This API enables embedding
 * such algorithms as nodes of FiniteFlow graphs.
 *
 * In general, an algorithm is defined by some functions that perform
 * the numerical evaluation as well as some data that may be accessed
 * during such evaluation.  The ffAlgNative() call defines a native
 * algorithm and takes, an inputs, a structure of function pointers
 * and pointers to the data.
 *
 *
 * The most important point to be aware of is that FiniteFlow, by
 * default, parallelizes the numerical evaluations across multiple
 * threads.  Hence, functions implementing the evaluation of native
 * algorithms may be called concurrently from multiple threads.  Some
 * care is therefore needed to avoid data races.  A data race may
 * happen if several threads concurrently access the same data and at
 * least one of them modifies it, causing undefined behaviour (unless
 * data access is protected by the user via some synchronization
 * mechanism, which however would generally deteriorate performance).
 *
 * FiniteFlow's approach to multi-threading consists of splitting
 * algorithms' data into two components: `shared_data` and
 * `mutable_data`.  The first, `shared_data`, is shared among all
 * evaluations of a node.  This is typically data that is not modified
 * during numerical evaluations and can thus be safely shared among
 * concurrent calls.  We recommend accessing this data via a `const`
 * pointer.  The second, `mutable_data`, is data that can be modified
 * during the numerical evaluations without any explicit
 * synchronization.  To avoid data races, each thread will use its own
 * copy of `mutable_data`.  If any `mutable_data` is needed, users
 * will have to provide a function that FiniteFlow will call to
 * "clone" (i.e. create a copy of) `mutable_data` for each additional
 * execution thread.  Note that calls to `clone` may also happen
 * concurrently in different threads, as they are not supposed to
 * modify `shared_data` or the source `mutable_data` but only creating
 * a new copy.  FiniteFlow guarantees that concurrent threads will
 * always use different copies of `mutable_data`.  Hence, as long as
 * the numerical evaluation does not modify `shared_data`,
 * thread-safety is guaranteed without any need to implement a
 * synchronization mechanism.
 *
 * We note that `shared_data` can be instead be freely modified during
 * the learning stage (see below), if present, since that always
 * happens in the main thread.
 *
 * If users need to modify `shared_data` during evaluations (or calls
 * to `clone`), it is up to them to ensure thread-safety.
 *
 *
 * Algorithms requiring a "learning" step must pass a non-zero value
 * for `min_learn_times` and also provide a `learn` function, which
 * will be called (at least) `min_learn_times` times during the
 * learning step.  The length of the output `nparsout` can be changed
 * by the `learn` function.
 */

#pragma once

#include <fflow/capi.h>
#include <fflow/ffmod.h>

#ifdef __cplusplus
extern "C" {
#endif

  ///////////////
  // Utilities //
  ///////////////

  // Use this to print general messages or debug information
  void ffDbgPrint(const char * str);

  // Use this to print error messages
  void ffLogErr(const char * str);


  //////////////////
  // Native nodes //
  //////////////////


  typedef struct FFContext FFContext;
  typedef const FFUInt * const FFAlgInput; // input lists

  // Information about the length of the inputs and the output of a
  // native algorithm.  This information is passed to all evaluation
  // functions, such that it does not need to be duplicated into
  // `shared_data`.
  typedef struct {
    const unsigned * nparsin;
    unsigned n_input_nodes;
    unsigned nparsout;
  } FFInOutInfo;

  // Information about the length of the inputs of a native algorithm.
  // This information is passed to the `learn` function, if any (see
  // also FFInOutInfo).  Note that the output length `nparsout` is
  // instead passed separately via a pointer and it can be modified by
  // the `learn` function.
  typedef struct {
    const unsigned * nparsin;
    unsigned n_input_nodes;
  } FFInInfo;

  typedef FFStatus (*FFEvaluateFun)(FFContext * ctxt, FFInOutInfo ioinfo,
                                    void * shared_data, void * mutable_data,
                                    FFAlgInput xin[], FFMod mod,
                                    FFUInt xout[]);
  typedef FFStatus (*FFLearnFun)(FFContext * ctxt, FFInInfo ininfo,
                                 void * shared_data, void * mutable_data,
                                 unsigned * nparsout,
                                 FFAlgInput xin[], FFMod mod);
  typedef void * (*FFCloneMutDataFun)(FFInOutInfo ioinfo,
                                      void * shared_data, void * mutable_data);
  typedef void (*FFDestroyMutDataFun)(void * mutable_data);
  typedef void (*FFDestroySharedDataFun)(FFInOutInfo ioinfo,
                                         void * shared_data);


  // The FFAlgFunctions structure contains function pointers to
  // user-defined functions that define a native algorithm.  Most of
  // the data members are optional and may have a NULL value.
  typedef struct {

    // always needed (it cannot be NULL)
    FFEvaluateFun evaluate;

    // needed if min_learn_times != 0
    FFLearnFun learn;

    // needed if mutable_data != NULL
    FFCloneMutDataFun clone_mutable_data;

    // optional function which is automatically called for each copy
    // of `mutable_data` when it is no longer needed
    FFDestroyMutDataFun destroy_mutable_data;

    // optional function which is automatically called when the node,
    // or the graph it belongs to, is deleted
    FFDestroySharedDataFun destroy_shared_data;

  } FFAlgFunctions;



  // This routine defines a native algorithm and embeds it as a node
  // in a graph.  The algorithm is defined by the function pointers in
  // the struct `funcs` and the data pointers `shared_data` and
  // `mutable_data`.
  //
  // The caller must make sure that the length of the output of the
  // specified input nodes is compatible with the expected one (see
  // ffNodeNParsOut).
  //
  // The pointer `shared_data` will be passed as a parameter to every
  // call of funcs.evaluate (and funcs.learn if needed).  The pointer
  // `mutable_data` or a pointer returned by funcs.clone_mutable_data
  // will also be passed to evaluation routines (see the discussion
  // above).
  //
  // The parameter `nparsout` must be the length of the output of the
  // evaluation.  If `min_learn_times != 0`, this length may only
  // become known during the learning step.  In such cases `nparsout`
  // may be set to zero and then modified during the calls to the
  // `funcs.learn` function.
  FFNode ffAlgNative(FFGraph graph,
                     const FFNode * input_nodes, unsigned n_input_nodes,
                     unsigned nparsout, unsigned min_learn_times,
                     void * shared_data, void * mutable_data,
                     FFAlgFunctions funcs);

  // Check if a node is a native extension
  bool ffAlgIsNative(FFGraph graph, FFNode node);


  // Call ffEnsureWorkspaceSize() when you create your node to
  // allocate some per-thread workspace (for all threads) and then
  // ffContextWorkspace() to access it during evaluation.  This is an
  // alternative to using mutable data in simple cases.  Note that the
  // workspace cannot be shrunk or freed and its content is not
  // retained across multiple evaluations.  Prefer using mutable_data
  // for large memory allocations, structured data or persistent data.
  // WARNING: Don't call ffEnsureWorkspaceSize() during numerical
  // evaluations, because the function is not thread safe.
  void ffEnsureWorkspaceSize(size_t n);
  FFUInt * ffContextWorkspace(FFContext * ctx);
  size_t ffWorkspaceSize(void);


  // You need to call this any time you modify the `mutable_data` in a
  // way that invalidates it.  This call will purge any existing copy
  // of the cloned algorithm data for that node.
  void ffInvalidateMutableData(FFGraph graph, FFNode node);


  // Access data of native algorithms.  Note that modifying data can
  // be error prone and requires some care and understanding of
  // mutability of nodes.  See also ffInvalidateMutableData().
  void * ffNativeSharedData(FFGraph graph, FFNode node);
  void * ffNativeMutableData(FFGraph graph, FFNode node);


  // After creating a node with ffAlgNative(), you can flag it with a
  // "type ID" to identify it as a node of a certain type.  This can
  // be useful, for instance, if you need to access the `shared_data`
  // and `mutable_data` with ffNativeSharedData() and
  // ffNativeMutableData() and cast it to its correct type.  You
  // obtain a "type ID" using ffNewNativeTypeID().  Each call of
  // ffNewNativeTypeID() will return a non-zero, unique type ID.
  // Then, after defining a native algorithm of a certain type, you
  // flag it with the corresponding type ID using
  // ffAssignNativeTypeID().  You can later check whether any node has
  // the type ID you expect using ffHasNativeTypeID().
  //
  // A simple example is:
  //
  //     static FFNativeTypeID my_native_type_id = 0;
  //
  //     // routine that creates native nodes of a certain type
  //     FFNode my_native_alg(FFGraph graph, ...)
  //     {
  //       /* ... some code ...  */
  //
  //       // get a unique "native type ID" if this is the first call
  //       if (my_native_type_id == 0)
  //         my_native_type_id = ffNewNativeTypeID();
  //
  //       FFNode node = ffAlgNative(graph,...);
  //       fAssignNativeTypeID(graph, node, my_native_type_id);
  //
  //       /* ... some more code ...  */
  //
  //       return node;
  //     }
  //
  //     // check whether a node is of my native type
  //     bool my_native_alg_check_type(FFGraph graph, FFNode node)
  //     {
  //       return ffHasNativeTypeID(graph, node, my_native_type_id);
  //     }
  //
  typedef unsigned FFNativeTypeID;
  FFNativeTypeID ffNewNativeTypeID(void);
  FFStatus ffAssignNativeTypeID(FFGraph graph, FFNode node, FFNativeTypeID id);
  bool ffHasNativeTypeID(FFGraph graph, FFNode node, FFNativeTypeID type);



  //////////////////////
  // Rational numbers //
  //////////////////////

  // We offer simple functions for multi-precision rational numbers,
  // backed by the multi-precision library used by FiniteFlow
  // (currently GMP).  These should be sufficient for most algorithms
  // over finite fields, which only need to parse rational numbers
  // from strings and then take their modulus w.r.t. a prime.  If you
  // need anything more complex than this, we recommend using a
  // dedicated library, such as GMP.

  // FFRatNumList represents a list of rational numbers
  typedef struct FFRatNumList FFRatNumList;

  // Obtain a list of rational numbers from an array of
  // null-terminated strings
  FFRatNumList * ffRatNumList(const FFCStr * ratnums, unsigned length);

  // Take a list of rational numbers modulus a prime `mod` and write
  // the result in `output`.  Note that the `output` array must be
  // long enough to store the result (see also ffRatNumListLen()).
  FFStatus ffRatNumListMod(const FFRatNumList * ratnums, FFMod mod,
                           FFUInt output[]);

  // Return the length of the list
  size_t ffRatNumListLen(const FFRatNumList * ratnums);

  // Deallocate the list of rational numbers
  void ffFreeRatNumList(FFRatNumList * ratnums);


#ifdef __cplusplus
}
#endif
