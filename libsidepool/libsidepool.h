#ifndef LIBSIDEPOOL_H_
#define LIBSIDEPOOL_H_

#include<stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** As a C library, libsidepool is single-threaded,
 * because of course, C sucks and nobody would write
 * multithreaded code in C unless they were at least
 * mildly insane.
 *
 * It is however strongly asynchronous in its design.
 * Almost everything that interfaces between the
 * libsidepool and the client uses callbacks.
 * This allows C programs that use libsidepool the
 * ability to handle several dozen thousand
 * connections if necessary, because honestly, any
 * ahead-of-time-compiled language is so fast in
 * practice (compared to I/O) that you do not need
 * no stinkin threads.
 *
 * If your language's runtime is so badly designed
 * as to use multithreads by default (e.g. Tokio on
 * Rust), then please note that you need to protect
 * ALL libsidepool calls with a mutex.
 * That includes callbacks given by the library to
 * client code.
 *
 * In short: you really ought to use a single-threaded
 * event loop mechanism, because that is what is
 * actually reasonable for most I/O-bound programs
 * anyway.
 *
 * Admittedly, some POSIX APIs suck and provide some
 * synchronous interface to an inherently network
 * operation, the canonical example being
 * `gethostbyname` (but there are TONS of this kind
 * of POSIX misdesign, like disk-file `fd`s never
 * being usefully `select`able/`poll`able even though
 * modern "disk files" may be an NFS mount).
 * The correct solution is to have a separate pool
 * of threads in a POSIX-me-harder threadpool, with
 * operations sent by the event loop to the
 * threadpool in a multithreaded 1PMC requests queue,
 * operation results sent by the POSIX-me-harder
 * threadpool to the event loop in a multithreaded
 * MP1C results queue, and a `pipe` from the
 * POSIX-me-harder threadpool to wake up the event
 * loop whenever some POSIX-me-harder thread
 * completes an operation and has pushed it to the
 * results queue.
 */

/** struct libsidepool
 *
 * @desc Top-level main object for libsidepool, which
 * manages sidepools for the node.
 *
 * This structure is opaque.
 */
struct libsidepool;

/** struct libsidepool_init
 *
 * @desc Structure for initializing a libsidepool
 * instance.
 * Either provide all the interfaces needed by
 * libsidepool and then instantiate with
 * `libsidepool_init_finish`, or cancel with
 * `libsidepool_init_cancel`.
 *
 * This structure is opaque.
 */
struct libsidepool_init;

/** libsidepool_init_start
 *
 * @desc Start the process of initializing a
 * libsidepool instance by returning a
 * `struct libsidepool_init` instance.
 * Free the initializer by either
 * `libsidepool_init_cancel` or finish and free it by
 * `libsidepool_init_finish`.
 *
 * @returns `NULL` and sets `errno` if there is an
 * error (usually `ENOMEM`), otherwise returns an
 * instance of `struct libsidepool_init`.
 */
/*gives*/ struct libsidepool_init*
libsidepool_init_start(void);

/** libsidepool_init_cancel
 *
 * @desc Free up the `struct libsidepool_init`
 * instance without constructing the complete
 * `struct libsidepool`.
 *
 * Any interface instances you have already passed
 * to the initialization instance will be freed by
 * this call.
 */
void libsidepool_init_cancel(
	/*takes*/ struct libsidepool_init*
);

/** struct libsidepool_randomizer
 *
 * @desc A client-provided interface structure which
 * gets high-quality random number from somewhere
 * (`getentropy`, `getrandom`, `read_random`,
 * `/dev/random`, `CryptRandom`, `SetRandom`,
 * etc).
 *
 * Ideally, if your OS thinks that it has
 * insufficient randomness and wants to block,
 * just let it block; this interface is
 * asynchronous so if the OS provides a
 * synchronous blocking interface (because
 * POSIX-me-harder) you should use the
 * POSIX-me-harder threadpool technique
 * described above.
 */
struct libsidepool_randomizer {
	/** A pointer to a user structure.
	 *
	 * The provider of this interface structure
	 * may use this pointer arbitrarily for any
	 * purpose, including leaving it
	 * uninitialized.
	 */
	void* user;
	/** If non-`NULL`, called on cancellation
	 * of `libsidepool_init` or freeing of the
	 * `struct libsidepool`, to free this
	 * randomizer.
	 */
	void (*free)(
		/*takes*/
		struct libsidepool_randomizer*
	);
	/** Called by the `libsidepool` library
	 * to get 32 bytes of high-quality
	 * random data.
	 *
	 * It is safe for this function to call
	 * `pass` synchronously, though if your
	 * OS provides a synchronous blocking
	 * interface you should really transform
	 * it into an asynchronous one.
	 */
	void (*rand)(
		/*borrows*/
		struct libsidepool_randomizer*,
		/** Callback to call after you
		 * have gotten 32 random bytes.
		 */
		/*borrows*/
		void (*pass)(
			/*takes*/void* context,
			/*borrows*/
			uint8_t random[32]
		),
		/** Callback to call if the
		 * randomizer is freed before
		 * you could get the random
		 * bytes.
		 */
		/*borrows*/
		void (*fail)(/*takes*/void* context),
		/** The context to pass to either
		 * pass or fail.
		 */
		/*takes*/
		void* context
	);
};

/** struct libsidepool_logger
 *
 * @desc A client-provided interface structure which
 * writes out logging strings.
 *
 * For each of the logging functions, the given
 * message string is only valid until the function
 * returns; you need to copy their contents if you
 * need to retain the information.
 */
struct libsidepool_logger {
	/** A pointer to a user structure.
	 *
	 * The provider of this interface structure
	 * may use this pointer arbitrarily for any
	 * purpose, including leaving it
	 * uninitialized.
	 */
	void* user;
	/** If non-`NULL`, called on cancellation of
	 * the `struct libsidepool_init`, or freeing
	 * of the `struct libsidepool`, to free this
	 * logger.
	 */
	void (*free)(
		/*takes*/ struct libsidepool_logger*
	);
	/** Print a log entry at debug level.  */
	void (*debug)(
		/*borrows*/ struct libsidepool_logger*,
		/*borrows*/ char const* msg
	);
	/** Print a log entry at info level.  */
	void (*info)(
		/*borrows*/ struct libsidepool_logger*,
		/*borrows*/ char const* msg
	);
	/** Print a log entry at warning level.  */
	void (*warning)(
		/*borrows*/ struct libsidepool_logger*,
		/*borrows*/ char const* msg
	);
	/** Print a log entry at error level.  */
	void (*error)(
		/*borrows*/ struct libsidepool_logger*,
		/*borrows*/ char const* msg
	);
};

/** libsidepool_init_set_logger
 *
 * @desc Give an instance of
 * `struct libsidepool_logger` to the given
 * initializer instance.
 *
 * After this call, the library is responsible for
 * calling the `free` method of the given logger
 * instance.
 */
void libsidepool_init_set_logger(
	/*borrows*/ struct libsidepool_init*,
	/*takes*/ struct libsidepool_logger*
);

/** struct libsidepool_idle_callback
 *
 * @desc A library-provided interface structure
 * provided to `struct libsidepool_idler` when
 * registering an idle callback.
 */
struct libsidepool_idle_callback {
	/** A pointer to a user structure.  */
	void* user;
	/** A non-`NULL` pointer to a function that is
	 * used to free this structure without invoking
	 * the idle callback.
	 * The client-provided idler interface MUST NOT
	 * call this function unless the interface gets
	 * freed with one or more idle callbacks not
	 * yet called.
	 */
	void (*free)(
		/*takes*/
		struct libsidepool_idle_callback*
	);
	/** A non-`NULL` pointer to a function that
	 * should be called by the client-provided
	 * `struct libsidepool_idler` when the
	 * process is otherwise idle.
	 *
	 * Once this function is called, responsibility
	 * for the `struct libsidepool_idle_callback`
	 * is now returned to the library and the
	 * library is responsible for freeing this
	 * callback object.
	 * This only needs to be called once.
	 *
	 * This may re-entrantly call the `on_idle`
	 * function of the idler.
	 */
	void (*invoke)(
		/*takes*/
		struct libsidepool_idle_callback*
	);

	/** Convenient, suggestively-named pointers
	 * for use by the client-provided idler.
	 * libsidepool will ignore these fields and
	 * not initialize, change, or read them.
	 *
	 * If you *do* use these in an embedded
	 * linked list, make sure to remove this
	 * node from the list *before* you call
	 * either `free` or `invoke`, as either
	 * of those will cause this object to
	 * be freed.
	 */
	struct libsidepool_idle_callback* prev;
	struct libsidepool_idle_callback* next;
};

/** struct libsidepool_idler
 *
 * @desc A client-provided interface structure which
 * allows a `struct libsidepool` instance to trigger a
 * function to be called, once, when the process is
 * idle.
 */
struct libsidepool_idler {
	/** A pointer to a user structure.  */
	void* user;
	/** If non-`NULL`, called on cancellation of
	 * the `struct libsidepool_init`, or freeing
	 * of the `struct libsidepool`, to free this
	 * idler.
	 */
	void (*free)(
		/*takes*/ struct libsidepool_idler*
	);
	/** A non-`NULL` function that is called by the
	 * library instance to add a
	 * `struct libsidepool_idle_callback` that is
	 * called while the process is idle.
	 *
	 * The client interface is responsible for the
	 * idle callback until it is able to call its
	 * `invoke` function or `free` function.
	 * It should only call the `free` function of
	 * any not-yet-triggered idle callbacks if its
	 * own `free` function is called.
	 *
	 * The given idle callback needs to be called
	 * exactly once, however, the callback may
	 * itself call `on_idle`.
	 *
	 * This function will be called only after the
	 * `libsidepool_init_finish` is called.
	 */
	void (*on_idle)(
		/*borrows*/ struct libsidepool_idler*,
		/*takes*/ struct libsidepool_idle_callback*
	);
};

/** libsidepool_init_set_idler
 *
 * @desc Give an instance of
 * `struct libsidepool_idler` to the given initializer
 * instance.
 *
 * After this call, the library is responsible for
 * freeing the idler instance.
 */
void libsidepool_init_set_idler(
	/*borrows*/ struct libsidepool_init*,
	/*takes*/ struct libsidepool_idler*
);

/* forward declaration.  */
struct libsidepool_saver_scan;
/** struct libsidepool_saver
 *
 * @desc A client-provided interface structure which
 * saves data to persistent storage.
 *
 * The saver model is that there is two hierarchies
 * of "directories", indexed by `key1` and `key2`.
 * Data is stored as `value` strings that should be
 * indexed by `key1,key2`.
 * In addition, it must be possible for the library
 * to give a `key1` and the saver should be able to
 * return the set of `key2`s inside that `key1`.
 * Finally, each `value` has an associated 32-bit
 * unsigned integer `generation`, which may start
 * at any value for initial insertion, but must be
 * monotonically increasing for each modification.
 *
 * See the CLN `lightning-datastore` for what the
 * model is based on.
 *
 * The `libsidepool` library promises that `key1`,
 * `key2`, and `value` are all proper NUL-terminated
 * C strings, and in addition, will only contain the
 * following characters:
 *
 * - `A`-`Z` `a`-`z` `0`-`9`
 * - `_` underscore, `-` dash
 * - `+` plus, `%` percent, `=` equals
 *
 * In particular `/` will never appear in `key1`,
 * `key2`, or `value`, so you can use a directory
 * structure `key1/key2` where the `key2` are files
 * containing the `generation` integer and `value`
 * string.
 * None of the characters above would require
 * escaping if put in a C or JSON string, as well.
 *
 * The saver must ensure atomicity and consistency.
 * Even if the saver is not able to call the
 * corresponding `pass` or `fail` callback, the
 * on-disk state must be either before any update,
 * or after, with no "halfway" changes, even in
 * event of a crash or unplanned process termination.
 *
 * The library assumes the saver is capable of
 * significant parallelism (i.e. the library will
 * happily call multiple functions without waiting
 * for them to call their callbacks).
 * Write your saver appropriately.
 *
 * Unfortunately for POSIX lovers, POSIX inherently
 * assumes that disk access is so fast that there is
 * no need to provide an asynchronous interface to
 * disk.
 * Doing `select`/`poll` on a disk-file `fd` results
 * in immediate signaling of readiness, as does
 * Linux `epoll` in general (supposedly opening a
 * disk-file with `O_NONBLOCK` would let `epoll`
 * work, but that may not work on, say, all
 * filesystems Linux supports, and Linux kernel
 * manfiles do not mention this).
 * However, in practice a really professional
 * Lightning Network node deployment would probably
 * put storage in some kind of replicated, possibly
 * network-mounted storage.
 * Thus, in practice you may need to use your own
 * threadpool (separate from thread(s) that handle
 * your event loop) that performs disk I/O, and
 * raise events in your event loop on completion
 * or error (a `pipe` from your POSIX-me-harder
 * threadpool to your event loop is usually
 * sufficient to integrate such a threadpool to
 * some single-threaded event loop).
 */
struct libsidepool_saver {
	/** A pointer to a user structure.  */
	void* user;
	/** If non-`NULL`, called on cancellaiton of
	 * the `struct libsidepool_init`, or freeing
	 * of the `struct libsidepool`, to free this
	 * saver.
	 *
	 * The library is responsible for freeing
	 * any live `libsidepool_saver_scan`
	 * instances, separately from the saver.
	 * Contrariwise, this implies that scan
	 * instances must be capable of operating
	 * even if the saver that created them has
	 * been freed.
	 *
	 * If `free` is called before one or more
	 * of the `scan`, `create`, `read`,`update`,
	 * or `delete` functions have called their
	 * callbacks, then they should call their
	 * `fail` callbacks with `ECANCELED`.
	 * As long as the underlying disk operation
	 * is atomic (either the operation succeeds
	 * or fails in full, no "halfsies" state)
	 * it is OK for the underlying operation to
	 * succeed but not get reported with `pass`
	 * if the saver has been `free`d.
	 */
	void (*free)(
		/*takes*/ struct libsidepool_saver*
	);
	/** Given a `key1`, list out all `key2`s
	 * inside that `key1`, returning a scan
	 * result object.
	 *
	 * If there are no entries with `key1`,
	 * this may either call `fail` with
	 * `ENOENT` or `pass` with a scan result
	 * object that represents an empty set.
	 *
	 * `key1` will be freed after this function
	 * returns; make sure to copy it if
	 * necessary.
	 */
	void (*scan)(
		/*borrows*/ struct libsidepool_saver*,
		/*borrows*/ char const* key1,
		/** Callback to call after the
		 * scan object is constructed.
		 */
		/*borrows*/
		void (*pass)(
			/*takes*/ void* context,
			/*takes*/
			struct libsidepool_saver_scan*
		),
		/** Callback to call if the
		 * scan object cannot be
		 * constructed.
		 * Call this as well if the
		 * interface is freed before
		 * you can call `pass`, with
		 * `ECANCELED` as the `my_errno`
		 * argument.
		 * (conversely, if you are
		 * failing for any other
		 * reason, you should not be
		 * using `ECANCELED`).
		 */
		/*borrows*/
		void (*fail)(
			/*takes*/
			void* context,
			int my_errno
		),
		/** Context to pass to the
		 * `pass` or `fail` callback.
		 */
		/*takes*/
		void* context
	);
	/** Create a new `key1,key2` entry,
	 * with the given `value` string, and
	 * set `generation` to the lowest value
	 * you want.
	 *
	 * The given `key1,key2` MUST NOT exist
	 * before this call;
	 * if it does, you should call `fail`
	 * with `EEXIST`.
	 *
	 * The given `key1`, `key2`, and `value`
	 * are only valid until this function
	 * returns; you should copy them if
	 * necessary.
	 */
	void (*create)(
		/*borrows*/ struct libsidepool_saver*,
		/*borrows*/ char const* key1,
		/*borrows*/ char const* key2,
		/*borrows*/ char const* value,
		/** Callback to call if the
		 * creation of the new entry
		 * succeeds and is reliably
		 * persisted.
		 */
		/*borrows*/
		void (*pass)(
			/*takes*/ void* context,
			uint32_t generation
		),
		/** Callback to call if the
		 * creation of the new entry
		 * fails.
		 * If an entry with the same
		 * `key1,key2` already exists,
		 * you should call this with
		 * `EEXIST`.
		 *
		 * If the saver is freed while
		 * a creation request is not
		 * yet sure to be completed,
		 * call this with `ECANCELED`.
		 */
		/*borrows*/
		void (*fail)(
			/*takes*/ void* context,
			int my_errno
		),
		/** Context for the `pass` or
		 * `fail` callbacks.
		 */
		/*takes*/ void* context
	);
	/** Read an existing entry at `key1,key2`.
	 *
	 * If the entry does not exist, call `fail`
	 * with `my_errno` set to `ENOENT`.
	 *
	 * The given `key1` and `key2` are only
	 * valid until this function returns;
	 * you should copy them.
	 */
	void (*read)(
		/*borrows*/ struct libsidepool_saver*,
		/*borrows*/ char const* key1,
		/*borrows*/ char const* key2,
		/** Callback to call if reading the
		 * given entry succeeds.
		 *
		 * The given `value` must remain
		 * valid until this function returns;
		 * the library promises to copy it
		 * as soon as this function is entered.
		 * You should free its storage when
		 * the `pass` function returns.
		 */
		/*borrows*/
		void (*pass)(
			/*takes*/ void* context,
			uint32_t generation,
			/*borrows*/ char const* value
		),
		/** Callback to call if failed to
		 * read the given entry.
		 *
		 * If no entry with the given
		 * `key1,key2` is found, call with
		 * `ENOENT`.
		 *
		 * If the saver is freed while a
		 * read is not yet completed, call
		 * the `fail` callback with
		 * `ECANCELED`.
		 */
		/*borrows*/
		void (*fail)(
			/*takes*/ void* context,
			int my_errno
		),
		/** Context for the `pass` and
		 * `fail` callbacks.
		 */
		/*takes*/ void* context
	);
	/** Update an existing `key1,key2` entry.
	 * The given `generation` must exactly
	 * match the on-disk `generation`; this
	 * check must be done atomically with the
	 * update.
	 *
	 * If this call passes, the `generation` of
	 * the entry must be at least one higher
	 * than the current `generation`.
	 *
	 * The entry must exist.
	 * If it does not exist, call `fail` with
	 * `ENOENT`.
	 *
	 * If the given `generation` does not
	 * match the `generation` of the entry,
	 * call `fail` with `EPERM`.
	 *
	 * If the saver is freed before the update
	 * is known to be completed, call `fail`
	 * with `ECANCELED`.
	 */
	void (*update)(
		/*borrows*/ struct libsidepool_saver*,
		/*borrows*/ char const* key1,
		/*borrows*/ char const* key2,
		uint32_t generation,
		/*borrows*/ char const* value,
		/** Callback to call if updating
		 * the entry succeeds.
		 *
		 * Call with the new `generation`,
		 * which must be at least 1 higher
		 * than the current generation.
		 */
		/*borrows*/
		void (*pass)(
			/*takes*/ void* context,
			uint32_t new_generation
		),
		/** Callback to call if updating
		 * fails.
		 *
		 * Call with `ENOENT` if there is
		 * no entry with `key1,key2`.
		 * Call with `EPERM` if the given
		 * `generation` does not exactly
		 * equal the on-disk `generation.
		 */
		/*borrows*/
		void (*fail)(
			/*takes*/ void* context,
			int my_errno
		),
		/** Context for the `pass` and
		 * `fail` callbacks.
		 */
		/*takes*/
		void* context
	);
	/** Delete an existing `key1,key2` entry
	 * The given `generation` must exactly
	 * match the on-disk `generation`; this
	 * check must be done atomically with the
	 * deletion.
	 *
	 * The entry must exist.
	 * If it does not exist, call `fail` with
	 * `ENOENT`.
	 *
	 * If the saver is freed before the deletion
	 * is known to have completed, this should
	 * call `fail` with `ECANCELED`.
	 *
	 * After deletion, this `key2` should no
	 * longer appear at `scan`.
	 */
	void (*del)(
		/*borrows*/ struct libsidepool_saver*,
		/*borrows*/ char const* key1,
		/*borrows*/ char const* key2,
		uint32_t generation,
		/** Callback to call if the deletion
		 * succeeds.
		 */
		/*borrows*/
		void (*pass)(
			/*takes*/ void* context
		),
		/** Callback to call if the deletion
		 * fails.
		 *
		 * Call with `ENOENT` if the entry
		 * does not exist, or `EPERM` if
		 * the entry does exist but the
		 * `generation` does not match.
		 */
		/*borrows*/
		void (*fail)(
			/*takes*/ void* context,
			int my_errno
		),
		/** Context for the `pass` and
		 * `fail` callbacks.
		 */
		/*takes*/ void* context
	);
};

/** struct libsidepool_saver_scan
 *
 * @desc A client-provided interface structure
 * which allows the library to scan for `key2`
 * entries inside some given `key1`.
 */
struct libsidepool_saver_scan {
	/** A pointer to a user structure.  */
	void* user;
	/** If non-`NULL`, called to free up
	 * this scan result.
	 */
	void (*free)(
		/*takes*/
		struct libsidepool_saver_scan*
	);
	/** A non-`NULL` pointer to a function
	 * that returns the next `key2` found
	 * inside `key1`, or returns `NULL` if
	 * there are no more `key2` inside the
	 * `key1`.
	 *
	 * The returned `key2` must be valid
	 * until the next call to `get` or
	 * until `free` is called.
	 */
	/*lends*/
	char const* (*get)(
		/*borrows*/
		struct libsidepool_saver_scan*
	);
};

/** libsidepool_init_set_saver
 *
 * @desc Give an instance of `struct libsidepool_saver`
 * to the given initializer instance.
 *
 * After this call, the library is responsible for
 * calling the `free` method of the given
 * saver instance.
 */
void libsidepool_init_set_saver(
	/*borrows*/ struct libsidepool_init*,
	/*takes*/ struct libsidepool_saver*
);

/** struct libsidepool_math
 *
 * @desc A client-provided interface structure which
 * performs SECP256K1 math.
 *
 * Unlike other interfaces, this is synchronous as
 * it is expected to be purely CPU with no network
 * or other hardware interfacing.
 *
 * Scalars are 256-bit big-endian arrays of 32
 * bytes.
 *
 * Points are compressed DER format.
 *
 * You might wonder: why does libsidepool not just
 * link to libsecp256k1 directly itself?
 *
 * The principle followed here is that the
 * cryptographic library should really be copied
 * by the application into its own source code
 * repo (and this library as well should be copied
 * by the application, as it also contains
 * cryptographic code).
 *
 * The reason for this is that if you do something
 * like link to a system libsecp256k1 or using
 * `git submodule` and referring to some github,
 * there is a risk that your repository-serving
 * code is compromised by some person who emails
 * the tired maintainer who wants to retire from
 * maintaining and insert hacked code.
 * So the onus is really on the application dev
 * to select and carefully audit the source code
 * of libsecp256k1 and ***copy*** it to their
 * own repository, to protect against
 * maintainer-get-tired attacks.
 */
struct libsidepool_math {
	/** A pointer to a user structure.  */
	void* user;
	/** If non-`NULL`, called on cancellaiton of
	 * the `struct libsidepool_init`, or freeing
	 * of the `struct libsidepool`, to free this
	 * math module.
	 */
	void (*free)(
		/*takes*/ struct libsidepool_math*
	);
	/** Determine if the scalar is less than the
	 * SECP256K1 order `p`.
	 * Return 0 if >= `p`, non-zero otherwise.
	 *
	 * This should still return non-zero if the
	 * scalar is all 0.
	 */
	int (*is_valid)(
		/*borrows*/ struct libsidepool_math*,
		/*borrows*/ uint8_t const scalar[32]
	);
	/** Determine if the point lies in the
	 * SECP256K1 curve.
	 * Return 0 if not on SECP256K1, non-zero
	 * otherwise.
	 */
	int (*is_valid_pt)(
		/*borrows*/ struct libsidepool_math*,
		/*borrows*/ uint8_t const point[33]
	);
	/** Add a scalar to another scalar in-place,
	 * modulo the SECP256K1 `p`.
	 *
	 * The scalars are in big-endian form.
	 */
	void (*add)(
		/*borrows*/ struct libsidepool_math*,
		/*borrows inout*/
		uint8_t scalar_to_increase[32],
		/*borrows*/
		uint8_t const scalar_to_add[32]
	);
	/** "Add" a point to another point (in the
	 * sense that is homomorphic to scalar
	 * addition).
	 * The operation is done in place.
	 *
	 * The points are in compressed DER form.
	 */
	void (*add_pt)(
		/*borrows*/ struct libsidepool_math*,
		/*borrows inout*/
		uint8_t point_to_increase[33],
		/*borrows*/
		uint8_t const point_to_add[33]
	);
	/** Negate a scalar in-place, modulo the
	 * SECP256K1 `p`.
	 */
	void (*negate)(
		/*borrows*/
		struct libsidepool_math*,
		/*borrows inout*/
		uint8_t scalar_to_negate[32]
	);
	/** "Negate" a point in-place, homomorphic
	 * to scalar negation.
	 */
	void (*negate_pt)(
		/*borrows*/
		struct libsidepool_math*,
		/*borrows*/
		uint8_t point_to_negate[33]
	);
	/** Multiply the two scalars in-place,
	 * modulo the SECP256K1 `p`.
	 */
	void (*mul)(
		/*borrows*/
		struct libsidepool_math*,
		/*borrows inout*/
		uint8_t scalar_to_mult[32],
		/*borrows*/
		uint8_t const scalar_to_mult_by[32]
	);
	/** "Multiply" a point in-place by the
	 * given scalar, homomorphic to scalar
	 * multiplication modulo `p`.
	 * Also known as multiplicative tweak.
	 */
	void (*mul_pt)(
		/*borrows*/
		struct libsidepool_math*,
		/*borrows inout*/
		uint8_t point_to_mult[33],
		/*borrows*/
		uint8_t const scalar_to_mult_by[32]
	);
};

/** libsidepool_init_set_math
 *
 * @desc Give an instance of `struct libsidepool_math`
 * to the given initializer instance.
 *
 * After this call, the library is responsible for
 * calling the `free` method of the given math
 * module instance.
 */
void libsidepool_init_set_math(
	/*borrows*/ struct libsidepool_init*,
	/*takes*/ struct libsidepool_math*
);

/** LIBSIDEPOOL_MESSAGE_ID
 *
 * @desc All messages received by the client
 * node, which has this message ID, MUST be
 * given to the `struct libsidepool` instance
 * of the client node, via the
 * `libsidepool_receive_message` API.
 */
#define LIBSIDEPOOL_MESSAGE_ID 53609

/** libsidepool_receive_message
 *
 * @desc The client must call this function on
 * its `struct libsidepool` instance if it
 * receives a message from any peer whose
 * BOLT8 message ID matches
 * `LIBSIDEPOOL_MESSAGE_ID`.
 */
void libsidepool_receive_message(
	/*borrows*/ struct libsidepool*,
	/** The public key / node ID of the peer
	 * that sent the message, in compressed
	 * DER format.
	 */
	uint8_t peer[33],
	/** The length of the message data payload,
	 * in bytes.  */
	size_t message_length,
	/** The message payload.
	 * This does NOT include the
	 * LIBSIDEPOOL_MESSAGE_ID message ID.
	 */
	/*borrows*/ uint8_t const* message
);

/** struct libsidepool_noder_sendmsg
 *
 * @desc A library-provided interface and data
 * structure that contains the specifications
 * of a message to be sent to a peer.
 */
struct libsidepool_noder_sendmsg {
	/** A pointer to a user structure.  */
	void* user;
	/** The node public key / node ID of the
	 * peer to send the message to, in compressed
	 * DER format.
	 *
	 * The client is responsible for attempting
	 * to connect to the peer so that it can
	 * send the message.
	 */
	uint8_t peer[33];
	/** The message ID to send.
	 *
	 * This will always be equal to
	 * LIBSIDEPOOL_MESSAGE_ID but is included
	 * here for completeness.
	 */
	uint16_t message_id;
	/** The length of the message data to send.  */
	size_t message_data_length;
	/** The actual message payload to send.
	 * Always non-`NULL`.
	 */
	uint8_t* message_data;
	/** Call if the client is unable to connect
	 * to the peer to send the message, or if
	 * the noder object is freed before we
	 * could confirm that the message has been
	 * enqueued for sending.
	 */
	void (*fail)(
		/*takes*/
		struct libsidepool_noder_sendmsg
	);
	/** Call if the client has enqueued the
	 * message for sending to the peer.
	 *
	 * As a simple detail of TCP, even if the
	 * client has sent the message out to the
	 * socket, the peer might not receive it
	 * before there is a network breakdown, and
	 * it may take a significant amount of
	 * time for the OS of the peer to notice
	 * the network break and report an error.
	 * Thus, it is fine if the client uses
	 * some kind of queue or buffer, as long
	 * as the messages are sent out ASAP (i.e.
	 * less than a half-dozen seconds, ideally
	 * less than a second).
	 * The library will handle the case where
	 * messages are not received by the peer,
	 * and the client does not need to worry
	 * as long as it has made a best-effort
	 * attempt to get the message to the peer.
	 *
	 * You SHOULD call this *after* you have
	 * established connection to the peer
	 * (if you do not have a connection to
	 * the peer yet).
	 */
	void (*pass)(
		/*takes*/
		struct libsidepool_noder_sendmsg
	);
};

/** struct libsidepool_noder_getchannel
 *
 * @desc A library-provided interface and data
 * structure that contains a request to list
 * the totals of all live channels with a
 * specific peer.
 *
 * The node should only report channels that
 * have been "locked in" (after `channel_ready`
 * has been exchanged by both peers) but before
 * it has seen any attempt to close / `shutdown`
 * or before it makes its own attempt to close
 * the chnnel.
 *
 * If the node software supports multiple
 * channels per peer, the noder object should
 * report each one with separate calls to
 * `report`.
 *
 * If the node has no live channel with the
 * peer, just call `finish` without calling
 * `report`.
 */
struct libsidepool_noder_getchannel {
	/** A pointer to a user structure.  */
	void* user;
	/** The node public key / node ID of the
	 * peer.
	 */
	uint8_t peer[33];
	/** Call this if the noder object is
	 * freed before you can complete the
	 * request.
	 *
	 * It is OK to call this after calling
	 * `report`.
	 */
	void (*free)(
		/*takes*/
		struct libsidepool_noder_getchannel*
	);
	/** Call this with the details of
	 * *one* live channel with the
	 * specified peer.
	 *
	 * - `capacity_sat != 0`
	 * - `our_side_msat + their_side_msat <= capacity_sat * 1000`
	 *
	 * Note that you MUST NOT compute
	 * `their_side_msat = capacity_sat * 1000 - our_side_msat`
	 * (or vice versa); HTLCs are neither
	 * ours nor theirs.
	 * You should be using the channel amounts
	 * that are singly ours and singly theirs.
	 *
	 * The amounts should also include any
	 * channel reserve requirements and any
	 * non-anchor fees.
	 */
	void (*report)(
		/*borrows*/
		struct libsidepool_noder_getchannel*,
		/** Total channel capacity, in
		 * satoshis.
		 * This is the amount of the UTXO
		 * backing this channel, if this
		 * were a normal LN BOLT channel.
		 */
		uint64_t capacity_sat,
		/** The amount in this channel that
		 * is unambiguously on our side.
		 * HTLCs MUST NOT be counted in
		 * either our side or their side.
		 * This should include any onchain
		 * fees and any reserve requirements.
		 */
		uint64_t our_side_msat,
		/** The amount in the channel that
		 * is unambiguously on their
		 * side.
		 * This should include any onchain
		 * fees and any reserve requirements.
		 */
		uint64_t their_side_msat
	);
	/** Call this after you have reported all
	 * channels with the specified peer.
	 */
	void (*finish)(
		/*takes*/
		struct libsidepool_noder_getchannel*
	);
};

/** struct libsidepool_noder_ping
 *
 * @desc A library-provided interface and data
 * structure that indicates to connect to,
 * and send a BOLT1 `ping` message, and wait
 * for a corresponding `pong`.
 */
struct libsidepool_noder_ping {
	/** A pointer to a user structure.  */
	void* user;
	/** The node public key / node ID of
	 * the peer to connect to and `ping`,
	 * and wait for a `pong`.
	 */
	uint8_t peer[33];
	/** The `num_pong_bytes` field to put
	 * in the `ping` message.
	 *
	 * `libsidepool` promises that for this
	 * version and for all future versions,
	 * this will be 0.
	 * It is only included for completeness.
	 */
	uint16_t num_pong_bytes;
	/** The time limit to wait for a `pong`.
	 * If you are unable to get the connection,
	 * send a `ping`, and get some `pong`
	 * from the peer with a 0-length payload.
	 * then call the `timed_out` function.
	 *
	 * This is in seconds since the Unix epoch,
	 * or at least whatever reference that
	 * `time` returns on your system.
	 */
	uint64_t timeout;
	/** Call this if the noder is freed
	 * before you could respond to the
	 * libsidepool.
	 */
	void (*free)(
		/*takes*/
		struct libsidepool_noder_ping*
	);
	/** Call this if the `timeout` has
	 * been reached or exceeded without the
	 * peer responding with `pong`.
	 */
	void (*fail)(
		/*takes*/
		struct libsidepool_noder_ping*
	);
	/** Call this upon receiving `pong` from
	 * the peer.
	 */
	void (*pass)(
		/*takes*/
		struct libsidepool_noder_ping*
	);
};

/* TODO: figure out HTLCs in the noder.

The problem is that LDK and CLN have different
hooks for HTLC acceptance.

CLN allows a plugin to hook into arbitrary
HTLCs you receive.
However, LDK will only trigger a hook if you
pre-register the HTLC hash, and it will give
you a `payment_secret` that has to appear
on the incoming HTLC, before it will let you
handle the HTLC.
And it is not clear to me that the incoming
HTLC would re-trigger the hook on restart
(CLN would re-trigger the hook).
*/
/* TODO: noder.  */

struct libsidepool_keykeeper_req;

/** struct libsidepool_keykeeper
 *
 * @desc A client-provided interface class that
 * is used to interact with a fixed client-owned
 * private key, without exposing the private key
 * to libsidepool.
 *
 * The keykeeper object must represent some
 * private high-entropy key that is constant
 * for all constructions of the `libsidepool`
 * object of the node.
 * For example, the keykeeper can be an object
 * that holds the private key of the Lightning
 * Network node ID of the node that is using
 * `libsidepool`.
 *
 * Basically, the key being kept must be
 * the same even across restarts of the node.
 * `libsidepool` will check this by storing
 * both the entropy and the public key
 * corresponding to the key returned by the
 * first-ever time it called into this interface,
 * and report an error in case of mismatch.
 *
 * This is still an asynchronous interface,
 * because ideally you would isolate the node
 * private key in a separate process at the very
 * least, and ideally on different hardware.
 */
struct libsidepool_keykeeper {
	/** A pointer to a user structure.  */
	void* user;
	/** If non-`NULL`, called on cancellation of
	 * the `struct libsidepool_init`, or freeing
	 * of the `struct libsidepool`, to free this
	 * keykeeper.
	 */
	void (*free)(
		/*takes*/ struct libsidepool_keykeeper*
	);
	/** Request a private key that is a
	 * deterministic function of the
	 * entropy in the given request and the
	 * client fixed private key.
	 *
	 * For a CLN node, this can be implemented
	 * via the `makesecret` RPC command.
	 */
	void (*request)(
		/*borrows*/
		struct libsidepool_keykeeper*,
		/*takes*/
		struct libsidepool_keykeeper_req*
	);
};

/** struct libsidepool_keykeeper_req
 *
 * @desc A libsidepool-provided interface and
 * data structure that contains the details of
 * the request to get a tweaked key.
 */
struct libsidepool_keykeeper_req {
	/** A pointer to a user structure.  */
	void* user;
	/** The tweak; some random non-secret
	 * entropy that should be fed to a
	 * deterministic function, together with
	 * the keykeeper secret key, to generate
	 * the tweaked_key.
	 * This is an input to the keykeeper
	 * `request` interface.
	 *
	 * You must use a deterministic trapdoor
	 * function that does not leak the
	 * fixed secret key held by the keykeeper.
	 * For example, you can use HMAC-SHA256
	 * with the keykeeper secret as the key
	 * and this tweak as the data.
	 */
	/*input*/ uint8_t tweak[32];
	/** The output result of taking the
	 * above tweak and the keykeeper secret
	 * and combining them into some deterministic
	 * trapdoor function.
	 * `libsidepool` initializes this to all 0s
	 * before calling into `request`.
	 */
	/*output*/ uint8_t tweaked_key[32];
	/** Call this if the keykeeper is freed
	 * before you could complete this request.
	 */
	void (*free)(
		/*takes*/
		struct libsidepool_keykeeper_req*
	);
	/** Call this after you have loaded the
	 * `tweaked_key` output.
	 *
	 * It is perfectly safe to call this
	 * directly from the keykeeper `request`
	 * function; `libsidepool` will copy the
	 * resulting tweaked key and then
	 * schedule the continuation of the process
	 * during an idle period via your provided
	 * idler.
	 * For example, if you keep the node private
	 * key in the memory of the process that
	 * also contains the `libsidepool` instance,
	 * and use the node private key as the
	 * fixed keykeeper secret, you do not need
	 * to wait, you can just call this immediately
	 * after you calculate the secret.
	 */
	void (*pass)(
		/*takes*/
		struct libsidepool_keykeeper_req*
	);
};

/** libsidepool_init_set_keykeeper
 *
 * @desc Give an instance of
 * `struct libsidepool_keykeeper` to the given
 * initializer instance.
 *
 * After this call, the library is responsible for
 * calling the `free` method of the given
 * keykeeper.
 */
void libsidepool_init_set_keykeeper(
	/*borrows*/ struct libsidepool_init*,
	/*takes*/ struct libsidepool_keykeeper*
);

#ifdef __cplusplus
}
#endif

#endif /* !defined(LIBSIDEPOOL_H_) */
