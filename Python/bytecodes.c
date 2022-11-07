// This file contains instruction definitions.
// It is read by Tools/cases_generator/generate_cases.py
// to generate Python/generated_cases.c.h.
// Note that there is some dummy C code at the top and bottom of the file
// to fool text editors like VS Code into believing this is valid C code.
// The actual instruction definitions start at // BEGIN BYTECODES //.
// See Tools/cases_generator/README.md for more information.

#include "Python.h"
#include "pycore_abstract.h"      // _PyIndex_Check()
#include "pycore_call.h"          // _PyObject_FastCallDictTstate()
#include "pycore_ceval.h"         // _PyEval_SignalAsyncExc()
#include "pycore_code.h"
#include "pycore_function.h"
#include "pycore_long.h"          // _PyLong_GetZero()
#include "pycore_object.h"        // _PyObject_GC_TRACK()
#include "pycore_moduleobject.h"  // PyModuleObject
#include "pycore_opcode.h"        // EXTRA_CASES
#include "pycore_pyerrors.h"      // _PyErr_Fetch()
#include "pycore_pymem.h"         // _PyMem_IsPtrFreed()
#include "pycore_pystate.h"       // _PyInterpreterState_GET()
#include "pycore_range.h"         // _PyRangeIterObject
#include "pycore_sliceobject.h"   // _PyBuildSlice_ConsumeRefs
#include "pycore_sysmodule.h"     // _PySys_Audit()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "pycore_emscripten_signal.h"  // _Py_CHECK_EMSCRIPTEN_SIGNALS

#include "pycore_dict.h"
#include "dictobject.h"
#include "pycore_frame.h"
#include "opcode.h"
#include "pydtrace.h"
#include "setobject.h"
#include "structmember.h"         // struct PyMemberDef, T_OFFSET_EX

void _PyFloat_ExactDealloc(PyObject *);
void _PyUnicode_ExactDealloc(PyObject *);

/* Stack effect macros
 * These will be mostly replaced by stack effect descriptions,
 * but the tooling need to recognize them.
 */
#define SET_TOP(v)        (stack_pointer[-1] = (v))
#define SET_SECOND(v)     (stack_pointer[-2] = (v))
#define PEEK(n)           (stack_pointer[-(n)])
#define PUSH(val)         (*(stack_pointer++) = (val))
#define POP()             (*(--stack_pointer))
#define TOP()             PEEK(1)
#define SECOND()          PEEK(2)
#define STACK_GROW(n)     (stack_pointer += (n))
#define STACK_SHRINK(n)   (stack_pointer -= (n))
#define EMPTY()           1
#define STACK_LEVEL()     2

/* Local variable macros */
#define GETLOCAL(i)     (frame->localsplus[i])
#define SETLOCAL(i, val)  \
do { \
    PyObject *_tmp = frame->localsplus[i]; \
    frame->localsplus[i] = (val); \
    Py_XDECREF(_tmp); \
} while (0)

/* Flow control macros */
#define DEOPT_IF(cond, instname) ((void)0)
#define JUMPBY(offset) ((void)0)
#define GO_TO_INSTRUCTION(instname) ((void)0)
#define DISPATCH_SAME_OPARG() ((void)0)
#define DISPATCH() ((void)0)

#define inst(name) case name:
#define super(name) static int SUPER_##name
#define family(name) static int family_##name

#define NAME_ERROR_MSG \
    "name '%.200s' is not defined"

typedef struct {
    PyObject *kwnames;
} CallShape;

static PyObject *
dummy_func(
    PyThreadState *tstate,
    _PyInterpreterFrame *frame,
    unsigned char opcode,
    unsigned int oparg,
    _Py_atomic_int * const eval_breaker,
    _PyCFrame cframe,
    PyObject *names,
    PyObject *consts,
    _Py_CODEUNIT *next_instr,
    PyObject **stack_pointer,
    CallShape call_shape,
    _Py_CODEUNIT *first_instr,
    int throwflag,
    binaryfunc binary_ops[]
)
{
    switch (opcode) {

        /* BEWARE!
           It is essential that any operation that fails must goto error
           and that all operation that succeed call DISPATCH() ! */

// BEGIN BYTECODES //
        // stack effect: ( -- )
        inst(NOP) {
        }

        // stack effect: ( -- )
        inst(RESUME) {
            assert(tstate->cframe == &cframe);
            assert(frame == cframe.current_frame);
            if (_Py_atomic_load_relaxed_int32(eval_breaker) && oparg < 2) {
                goto handle_eval_breaker;
            }
        }

        // stack effect: ( -- __0)
        inst(LOAD_CLOSURE) {
            /* We keep LOAD_CLOSURE so that the bytecode stays more readable. */
            PyObject *value = GETLOCAL(oparg);
            if (value == NULL) {
                goto unbound_local_error;
            }
            Py_INCREF(value);
            PUSH(value);
        }

        // stack effect: ( -- __0)
        inst(LOAD_FAST_CHECK) {
            PyObject *value = GETLOCAL(oparg);
            if (value == NULL) {
                goto unbound_local_error;
            }
            Py_INCREF(value);
            PUSH(value);
        }

        // stack effect: ( -- __0)
        inst(LOAD_FAST) {
            PyObject *value = GETLOCAL(oparg);
            assert(value != NULL);
            Py_INCREF(value);
            PUSH(value);
        }

        // stack effect: ( -- __0)
        inst(LOAD_CONST) {
            PyObject *value = GETITEM(consts, oparg);
            Py_INCREF(value);
            PUSH(value);
        }

        // stack effect: (__0 -- )
        inst(STORE_FAST) {
            PyObject *value = POP();
            SETLOCAL(oparg, value);
        }

        super(LOAD_FAST__LOAD_FAST) = LOAD_FAST + LOAD_FAST;
        super(LOAD_FAST__LOAD_CONST) = LOAD_FAST + LOAD_CONST;
        super(STORE_FAST__LOAD_FAST)  = STORE_FAST + LOAD_FAST;
        super(STORE_FAST__STORE_FAST) = STORE_FAST + STORE_FAST;
        super (LOAD_CONST__LOAD_FAST) = LOAD_CONST + LOAD_FAST;

        // stack effect: (__0 -- )
        inst(POP_TOP) {
            PyObject *value = POP();
            Py_DECREF(value);
        }

        // stack effect: ( -- __0)
        inst(PUSH_NULL) {
            /* Use BASIC_PUSH as NULL is not a valid object pointer */
            BASIC_PUSH(NULL);
        }

        // stack effect: (__0, __1 -- )
        inst(END_FOR) {
            PyObject *value = POP();
            Py_DECREF(value);
            value = POP();
            Py_DECREF(value);
        }

        // stack effect: ( -- )
        inst(UNARY_POSITIVE) {
            PyObject *value = TOP();
            PyObject *res = PyNumber_Positive(value);
            Py_DECREF(value);
            SET_TOP(res);
            if (res == NULL)
                goto error;
        }

        // stack effect: ( -- )
        inst(UNARY_NEGATIVE) {
            PyObject *value = TOP();
            PyObject *res = PyNumber_Negative(value);
            Py_DECREF(value);
            SET_TOP(res);
            if (res == NULL)
                goto error;
        }

        // stack effect: ( -- )
        inst(UNARY_NOT) {
            PyObject *value = TOP();
            int err = PyObject_IsTrue(value);
            Py_DECREF(value);
            if (err == 0) {
                Py_INCREF(Py_True);
                SET_TOP(Py_True);
                DISPATCH();
            }
            else if (err > 0) {
                Py_INCREF(Py_False);
                SET_TOP(Py_False);
                DISPATCH();
            }
            STACK_SHRINK(1);
            goto error;
        }

        // stack effect: ( -- )
        inst(UNARY_INVERT) {
            PyObject *value = TOP();
            PyObject *res = PyNumber_Invert(value);
            Py_DECREF(value);
            SET_TOP(res);
            if (res == NULL)
                goto error;
        }

        // stack effect: (__0 -- )
        inst(BINARY_OP_MULTIPLY_INT) {
            assert(cframe.use_tracing == 0);
            PyObject *left = SECOND();
            PyObject *right = TOP();
            DEOPT_IF(!PyLong_CheckExact(left), BINARY_OP);
            DEOPT_IF(!PyLong_CheckExact(right), BINARY_OP);
            STAT_INC(BINARY_OP, hit);
            PyObject *prod = _PyLong_Multiply((PyLongObject *)left, (PyLongObject *)right);
            SET_SECOND(prod);
            _Py_DECREF_SPECIALIZED(right, (destructor)PyObject_Free);
            _Py_DECREF_SPECIALIZED(left, (destructor)PyObject_Free);
            STACK_SHRINK(1);
            if (prod == NULL) {
                goto error;
            }
            JUMPBY(INLINE_CACHE_ENTRIES_BINARY_OP);
        }

        // stack effect: (__0 -- )
        inst(BINARY_OP_MULTIPLY_FLOAT) {
            assert(cframe.use_tracing == 0);
            PyObject *left = SECOND();
            PyObject *right = TOP();
            DEOPT_IF(!PyFloat_CheckExact(left), BINARY_OP);
            DEOPT_IF(!PyFloat_CheckExact(right), BINARY_OP);
            STAT_INC(BINARY_OP, hit);
            double dprod = ((PyFloatObject *)left)->ob_fval *
                ((PyFloatObject *)right)->ob_fval;
            PyObject *prod = PyFloat_FromDouble(dprod);
            SET_SECOND(prod);
            _Py_DECREF_SPECIALIZED(right, _PyFloat_ExactDealloc);
            _Py_DECREF_SPECIALIZED(left, _PyFloat_ExactDealloc);
            STACK_SHRINK(1);
            if (prod == NULL) {
                goto error;
            }
            JUMPBY(INLINE_CACHE_ENTRIES_BINARY_OP);
        }

        // stack effect: (__0 -- )
        inst(BINARY_OP_SUBTRACT_INT) {
            assert(cframe.use_tracing == 0);
            PyObject *left = SECOND();
            PyObject *right = TOP();
            DEOPT_IF(!PyLong_CheckExact(left), BINARY_OP);
            DEOPT_IF(!PyLong_CheckExact(right), BINARY_OP);
            STAT_INC(BINARY_OP, hit);
            PyObject *sub = _PyLong_Subtract((PyLongObject *)left, (PyLongObject *)right);
            SET_SECOND(sub);
            _Py_DECREF_SPECIALIZED(right, (destructor)PyObject_Free);
            _Py_DECREF_SPECIALIZED(left, (destructor)PyObject_Free);
            STACK_SHRINK(1);
            if (sub == NULL) {
                goto error;
            }
            JUMPBY(INLINE_CACHE_ENTRIES_BINARY_OP);
        }

        // stack effect: (__0 -- )
        inst(BINARY_OP_SUBTRACT_FLOAT) {
            assert(cframe.use_tracing == 0);
            PyObject *left = SECOND();
            PyObject *right = TOP();
            DEOPT_IF(!PyFloat_CheckExact(left), BINARY_OP);
            DEOPT_IF(!PyFloat_CheckExact(right), BINARY_OP);
            STAT_INC(BINARY_OP, hit);
            double dsub = ((PyFloatObject *)left)->ob_fval - ((PyFloatObject *)right)->ob_fval;
            PyObject *sub = PyFloat_FromDouble(dsub);
            SET_SECOND(sub);
            _Py_DECREF_SPECIALIZED(right, _PyFloat_ExactDealloc);
            _Py_DECREF_SPECIALIZED(left, _PyFloat_ExactDealloc);
            STACK_SHRINK(1);
            if (sub == NULL) {
                goto error;
            }
            JUMPBY(INLINE_CACHE_ENTRIES_BINARY_OP);
        }

        // stack effect: (__0 -- )
        inst(BINARY_OP_ADD_UNICODE) {
            assert(cframe.use_tracing == 0);
            PyObject *left = SECOND();
            PyObject *right = TOP();
            DEOPT_IF(!PyUnicode_CheckExact(left), BINARY_OP);
            DEOPT_IF(Py_TYPE(right) != Py_TYPE(left), BINARY_OP);
            STAT_INC(BINARY_OP, hit);
            PyObject *res = PyUnicode_Concat(left, right);
            STACK_SHRINK(1);
            SET_TOP(res);
            _Py_DECREF_SPECIALIZED(left, _PyUnicode_ExactDealloc);
            _Py_DECREF_SPECIALIZED(right, _PyUnicode_ExactDealloc);
            if (TOP() == NULL) {
                goto error;
            }
            JUMPBY(INLINE_CACHE_ENTRIES_BINARY_OP);
        }

        // stack effect: (__0 -- )
        inst(BINARY_OP_INPLACE_ADD_UNICODE) {
            assert(cframe.use_tracing == 0);
            PyObject *left = SECOND();
            PyObject *right = TOP();
            DEOPT_IF(!PyUnicode_CheckExact(left), BINARY_OP);
            DEOPT_IF(Py_TYPE(right) != Py_TYPE(left), BINARY_OP);
            _Py_CODEUNIT true_next = next_instr[INLINE_CACHE_ENTRIES_BINARY_OP];
            assert(_Py_OPCODE(true_next) == STORE_FAST ||
                   _Py_OPCODE(true_next) == STORE_FAST__LOAD_FAST);
            PyObject **target_local = &GETLOCAL(_Py_OPARG(true_next));
            DEOPT_IF(*target_local != left, BINARY_OP);
            STAT_INC(BINARY_OP, hit);
            /* Handle `left = left + right` or `left += right` for str.
             *
             * When possible, extend `left` in place rather than
             * allocating a new PyUnicodeObject. This attempts to avoid
             * quadratic behavior when one neglects to use str.join().
             *
             * If `left` has only two references remaining (one from
             * the stack, one in the locals), DECREFing `left` leaves
             * only the locals reference, so PyUnicode_Append knows
             * that the string is safe to mutate.
             */
            assert(Py_REFCNT(left) >= 2);
            _Py_DECREF_NO_DEALLOC(left);
            STACK_SHRINK(2);
            PyUnicode_Append(target_local, right);
            _Py_DECREF_SPECIALIZED(right, _PyUnicode_ExactDealloc);
            if (*target_local == NULL) {
                goto error;
            }
            // The STORE_FAST is already done.
            JUMPBY(INLINE_CACHE_ENTRIES_BINARY_OP + 1);
        }

        // stack effect: (__0 -- )
        inst(BINARY_OP_ADD_FLOAT) {
            assert(cframe.use_tracing == 0);
            PyObject *left = SECOND();
            PyObject *right = TOP();
            DEOPT_IF(!PyFloat_CheckExact(left), BINARY_OP);
            DEOPT_IF(Py_TYPE(right) != Py_TYPE(left), BINARY_OP);
            STAT_INC(BINARY_OP, hit);
            double dsum = ((PyFloatObject *)left)->ob_fval +
                ((PyFloatObject *)right)->ob_fval;
            PyObject *sum = PyFloat_FromDouble(dsum);
            SET_SECOND(sum);
            _Py_DECREF_SPECIALIZED(right, _PyFloat_ExactDealloc);
            _Py_DECREF_SPECIALIZED(left, _PyFloat_ExactDealloc);
            STACK_SHRINK(1);
            if (sum == NULL) {
                goto error;
            }
            JUMPBY(INLINE_CACHE_ENTRIES_BINARY_OP);
        }

        // stack effect: (__0 -- )
        inst(BINARY_OP_ADD_INT) {
            assert(cframe.use_tracing == 0);
            PyObject *left = SECOND();
            PyObject *right = TOP();
            DEOPT_IF(!PyLong_CheckExact(left), BINARY_OP);
            DEOPT_IF(Py_TYPE(right) != Py_TYPE(left), BINARY_OP);
            STAT_INC(BINARY_OP, hit);
            PyObject *sum = _PyLong_Add((PyLongObject *)left, (PyLongObject *)right);
            SET_SECOND(sum);
            _Py_DECREF_SPECIALIZED(right, (destructor)PyObject_Free);
            _Py_DECREF_SPECIALIZED(left, (destructor)PyObject_Free);
            STACK_SHRINK(1);
            if (sum == NULL) {
                goto error;
            }
            JUMPBY(INLINE_CACHE_ENTRIES_BINARY_OP);
        }

        // stack effect: (__0 -- )
        inst(BINARY_SUBSCR) {
            PyObject *sub = POP();
            PyObject *container = TOP();
            PyObject *res = PyObject_GetItem(container, sub);
            Py_DECREF(container);
            Py_DECREF(sub);
            SET_TOP(res);
            if (res == NULL)
                goto error;
            JUMPBY(INLINE_CACHE_ENTRIES_BINARY_SUBSCR);
        }

        // stack effect: (__0, __1 -- )
        inst(BINARY_SLICE) {
            PyObject *stop = POP();
            PyObject *start = POP();
            PyObject *container = TOP();

            PyObject *slice = _PyBuildSlice_ConsumeRefs(start, stop);
            if (slice == NULL) {
                goto error;
            }
            PyObject *res = PyObject_GetItem(container, slice);
            Py_DECREF(slice);
            if (res == NULL) {
                goto error;
            }
            SET_TOP(res);
            Py_DECREF(container);
        }

        // stack effect: (__0, __1, __2, __3 -- )
        inst(STORE_SLICE) {
            PyObject *stop = POP();
            PyObject *start = POP();
            PyObject *container = TOP();
            PyObject *v = SECOND();

            PyObject *slice = _PyBuildSlice_ConsumeRefs(start, stop);
            if (slice == NULL) {
                goto error;
            }
            int err = PyObject_SetItem(container, slice, v);
            Py_DECREF(slice);
            if (err) {
                goto error;
            }
            STACK_SHRINK(2);
            Py_DECREF(v);
            Py_DECREF(container);
        }

        // stack effect: (__0 -- )
        inst(BINARY_SUBSCR_ADAPTIVE) {
            _PyBinarySubscrCache *cache = (_PyBinarySubscrCache *)next_instr;
            if (ADAPTIVE_COUNTER_IS_ZERO(cache)) {
                PyObject *sub = TOP();
                PyObject *container = SECOND();
                next_instr--;
                if (_Py_Specialize_BinarySubscr(container, sub, next_instr) < 0) {
                    goto error;
                }
                DISPATCH_SAME_OPARG();
            }
            else {
                STAT_INC(BINARY_SUBSCR, deferred);
                DECREMENT_ADAPTIVE_COUNTER(cache);
                GO_TO_INSTRUCTION(BINARY_SUBSCR);
            }
        }

        // stack effect: (__0 -- )
        inst(BINARY_SUBSCR_LIST_INT) {
            assert(cframe.use_tracing == 0);
            PyObject *sub = TOP();
            PyObject *list = SECOND();
            DEOPT_IF(!PyLong_CheckExact(sub), BINARY_SUBSCR);
            DEOPT_IF(!PyList_CheckExact(list), BINARY_SUBSCR);

            // Deopt unless 0 <= sub < PyList_Size(list)
            Py_ssize_t signed_magnitude = Py_SIZE(sub);
            DEOPT_IF(((size_t)signed_magnitude) > 1, BINARY_SUBSCR);
            assert(((PyLongObject *)_PyLong_GetZero())->ob_digit[0] == 0);
            Py_ssize_t index = ((PyLongObject*)sub)->ob_digit[0];
            DEOPT_IF(index >= PyList_GET_SIZE(list), BINARY_SUBSCR);
            STAT_INC(BINARY_SUBSCR, hit);
            PyObject *res = PyList_GET_ITEM(list, index);
            assert(res != NULL);
            Py_INCREF(res);
            STACK_SHRINK(1);
            _Py_DECREF_SPECIALIZED(sub, (destructor)PyObject_Free);
            SET_TOP(res);
            Py_DECREF(list);
            JUMPBY(INLINE_CACHE_ENTRIES_BINARY_SUBSCR);
        }

        // stack effect: (__0 -- )
        inst(BINARY_SUBSCR_TUPLE_INT) {
            assert(cframe.use_tracing == 0);
            PyObject *sub = TOP();
            PyObject *tuple = SECOND();
            DEOPT_IF(!PyLong_CheckExact(sub), BINARY_SUBSCR);
            DEOPT_IF(!PyTuple_CheckExact(tuple), BINARY_SUBSCR);

            // Deopt unless 0 <= sub < PyTuple_Size(list)
            Py_ssize_t signed_magnitude = Py_SIZE(sub);
            DEOPT_IF(((size_t)signed_magnitude) > 1, BINARY_SUBSCR);
            assert(((PyLongObject *)_PyLong_GetZero())->ob_digit[0] == 0);
            Py_ssize_t index = ((PyLongObject*)sub)->ob_digit[0];
            DEOPT_IF(index >= PyTuple_GET_SIZE(tuple), BINARY_SUBSCR);
            STAT_INC(BINARY_SUBSCR, hit);
            PyObject *res = PyTuple_GET_ITEM(tuple, index);
            assert(res != NULL);
            Py_INCREF(res);
            STACK_SHRINK(1);
            _Py_DECREF_SPECIALIZED(sub, (destructor)PyObject_Free);
            SET_TOP(res);
            Py_DECREF(tuple);
            JUMPBY(INLINE_CACHE_ENTRIES_BINARY_SUBSCR);
        }

        // stack effect: (__0 -- )
        inst(BINARY_SUBSCR_DICT) {
            assert(cframe.use_tracing == 0);
            PyObject *dict = SECOND();
            DEOPT_IF(!PyDict_CheckExact(SECOND()), BINARY_SUBSCR);
            STAT_INC(BINARY_SUBSCR, hit);
            PyObject *sub = TOP();
            PyObject *res = PyDict_GetItemWithError(dict, sub);
            if (res == NULL) {
                if (!_PyErr_Occurred(tstate)) {
                    _PyErr_SetKeyError(sub);
                }
                goto error;
            }
            Py_INCREF(res);
            STACK_SHRINK(1);
            Py_DECREF(sub);
            SET_TOP(res);
            Py_DECREF(dict);
            JUMPBY(INLINE_CACHE_ENTRIES_BINARY_SUBSCR);
        }

        // stack effect: (__0 -- )
        inst(BINARY_SUBSCR_GETITEM) {
            PyObject *sub = TOP();
            PyObject *container = SECOND();
            _PyBinarySubscrCache *cache = (_PyBinarySubscrCache *)next_instr;
            uint32_t type_version = read_u32(cache->type_version);
            PyTypeObject *tp = Py_TYPE(container);
            DEOPT_IF(tp->tp_version_tag != type_version, BINARY_SUBSCR);
            assert(tp->tp_flags & Py_TPFLAGS_HEAPTYPE);
            PyObject *cached = ((PyHeapTypeObject *)tp)->_spec_cache.getitem;
            assert(PyFunction_Check(cached));
            PyFunctionObject *getitem = (PyFunctionObject *)cached;
            DEOPT_IF(getitem->func_version != cache->func_version, BINARY_SUBSCR);
            PyCodeObject *code = (PyCodeObject *)getitem->func_code;
            assert(code->co_argcount == 2);
            DEOPT_IF(!_PyThreadState_HasStackSpace(tstate, code->co_framesize), BINARY_SUBSCR);
            STAT_INC(BINARY_SUBSCR, hit);
            Py_INCREF(getitem);
            _PyInterpreterFrame *new_frame = _PyFrame_PushUnchecked(tstate, getitem);
            STACK_SHRINK(2);
            new_frame->localsplus[0] = container;
            new_frame->localsplus[1] = sub;
            for (int i = 2; i < code->co_nlocalsplus; i++) {
                new_frame->localsplus[i] = NULL;
            }
            _PyFrame_SetStackPointer(frame, stack_pointer);
            JUMPBY(INLINE_CACHE_ENTRIES_BINARY_SUBSCR);
            frame->prev_instr = next_instr - 1;
            new_frame->previous = frame;
            frame = cframe.current_frame = new_frame;
            CALL_STAT_INC(inlined_py_calls);
            goto start_frame;
        }

        // stack effect: (__0 -- )
        inst(LIST_APPEND) {
            PyObject *v = POP();
            PyObject *list = PEEK(oparg);
            if (_PyList_AppendTakeRef((PyListObject *)list, v) < 0)
                goto error;
            PREDICT(JUMP_BACKWARD);
        }

        // stack effect: (__0 -- )
        inst(SET_ADD) {
            PyObject *v = POP();
            PyObject *set = PEEK(oparg);
            int err;
            err = PySet_Add(set, v);
            Py_DECREF(v);
            if (err != 0)
                goto error;
            PREDICT(JUMP_BACKWARD);
        }

        // stack effect: (__0, __1, __2 -- )
        inst(STORE_SUBSCR) {
            PyObject *sub = TOP();
            PyObject *container = SECOND();
            PyObject *v = THIRD();
            int err;
            STACK_SHRINK(3);
            /* container[sub] = v */
            err = PyObject_SetItem(container, sub, v);
            Py_DECREF(v);
            Py_DECREF(container);
            Py_DECREF(sub);
            if (err != 0) {
                goto error;
            }
            JUMPBY(INLINE_CACHE_ENTRIES_STORE_SUBSCR);
        }

        // stack effect: (__0, __1, __2 -- )
        inst(STORE_SUBSCR_ADAPTIVE) {
            _PyStoreSubscrCache *cache = (_PyStoreSubscrCache *)next_instr;
            if (ADAPTIVE_COUNTER_IS_ZERO(cache)) {
                PyObject *sub = TOP();
                PyObject *container = SECOND();
                next_instr--;
                if (_Py_Specialize_StoreSubscr(container, sub, next_instr) < 0) {
                    goto error;
                }
                DISPATCH_SAME_OPARG();
            }
            else {
                STAT_INC(STORE_SUBSCR, deferred);
                DECREMENT_ADAPTIVE_COUNTER(cache);
                GO_TO_INSTRUCTION(STORE_SUBSCR);
            }
        }

        // stack effect: (__0, __1, __2 -- )
        inst(STORE_SUBSCR_LIST_INT) {
            assert(cframe.use_tracing == 0);
            PyObject *sub = TOP();
            PyObject *list = SECOND();
            PyObject *value = THIRD();
            DEOPT_IF(!PyLong_CheckExact(sub), STORE_SUBSCR);
            DEOPT_IF(!PyList_CheckExact(list), STORE_SUBSCR);

            // Ensure nonnegative, zero-or-one-digit ints.
            DEOPT_IF(((size_t)Py_SIZE(sub)) > 1, STORE_SUBSCR);
            Py_ssize_t index = ((PyLongObject*)sub)->ob_digit[0];
            // Ensure index < len(list)
            DEOPT_IF(index >= PyList_GET_SIZE(list), STORE_SUBSCR);
            STAT_INC(STORE_SUBSCR, hit);

            PyObject *old_value = PyList_GET_ITEM(list, index);
            PyList_SET_ITEM(list, index, value);
            STACK_SHRINK(3);
            assert(old_value != NULL);
            Py_DECREF(old_value);
            _Py_DECREF_SPECIALIZED(sub, (destructor)PyObject_Free);
            Py_DECREF(list);
            JUMPBY(INLINE_CACHE_ENTRIES_STORE_SUBSCR);
        }

        // stack effect: (__0, __1, __2 -- )
        inst(STORE_SUBSCR_DICT) {
            assert(cframe.use_tracing == 0);
            PyObject *sub = TOP();
            PyObject *dict = SECOND();
            PyObject *value = THIRD();
            DEOPT_IF(!PyDict_CheckExact(dict), STORE_SUBSCR);
            STACK_SHRINK(3);
            STAT_INC(STORE_SUBSCR, hit);
            int err = _PyDict_SetItem_Take2((PyDictObject *)dict, sub, value);
            Py_DECREF(dict);
            if (err != 0) {
                goto error;
            }
            JUMPBY(INLINE_CACHE_ENTRIES_STORE_SUBSCR);
        }

        // stack effect: (__0, __1 -- )
        inst(DELETE_SUBSCR) {
            PyObject *sub = TOP();
            PyObject *container = SECOND();
            int err;
            STACK_SHRINK(2);
            /* del container[sub] */
            err = PyObject_DelItem(container, sub);
            Py_DECREF(container);
            Py_DECREF(sub);
            if (err != 0)
                goto error;
        }

        // stack effect: (__0 -- )
        inst(PRINT_EXPR) {
            PyObject *value = POP();
            PyObject *hook = _PySys_GetAttr(tstate, &_Py_ID(displayhook));
            PyObject *res;
            if (hook == NULL) {
                _PyErr_SetString(tstate, PyExc_RuntimeError,
                                 "lost sys.displayhook");
                Py_DECREF(value);
                goto error;
            }
            res = PyObject_CallOneArg(hook, value);
            Py_DECREF(value);
            if (res == NULL)
                goto error;
            Py_DECREF(res);
        }

        // stack effect: (__array[oparg] -- )
        inst(RAISE_VARARGS) {
            PyObject *cause = NULL, *exc = NULL;
            switch (oparg) {
            case 2:
                cause = POP(); /* cause */
                /* fall through */
            case 1:
                exc = POP(); /* exc */
                /* fall through */
            case 0:
                if (do_raise(tstate, exc, cause)) {
                    goto exception_unwind;
                }
                break;
            default:
                _PyErr_SetString(tstate, PyExc_SystemError,
                                 "bad RAISE_VARARGS oparg");
                break;
            }
            goto error;
        }

        // stack effect: (__0 -- )
        inst(RETURN_VALUE) {
            PyObject *retval = POP();
            assert(EMPTY());
            _PyFrame_SetStackPointer(frame, stack_pointer);
            TRACE_FUNCTION_EXIT();
            DTRACE_FUNCTION_EXIT();
            _Py_LeaveRecursiveCallPy(tstate);
            if (!frame->is_entry) {
                frame = cframe.current_frame = pop_frame(tstate, frame);
                _PyFrame_StackPush(frame, retval);
                goto resume_frame;
            }
            _Py_LeaveRecursiveCallTstate(tstate);
            if (frame->owner == FRAME_OWNED_BY_GENERATOR) {
                PyGenObject *gen = _PyFrame_GetGenerator(frame);
                tstate->exc_info = gen->gi_exc_state.previous_item;
                gen->gi_exc_state.previous_item = NULL;
            }
            /* Restore previous cframe and return. */
            tstate->cframe = cframe.previous;
            tstate->cframe->use_tracing = cframe.use_tracing;
            assert(tstate->cframe->current_frame == frame->previous);
            assert(!_PyErr_Occurred(tstate));
            return retval;
        }

        // stack effect: ( -- )
        inst(GET_AITER) {
            unaryfunc getter = NULL;
            PyObject *iter = NULL;
            PyObject *obj = TOP();
            PyTypeObject *type = Py_TYPE(obj);

            if (type->tp_as_async != NULL) {
                getter = type->tp_as_async->am_aiter;
            }

            if (getter != NULL) {
                iter = (*getter)(obj);
                Py_DECREF(obj);
                if (iter == NULL) {
                    SET_TOP(NULL);
                    goto error;
                }
            }
            else {
                SET_TOP(NULL);
                _PyErr_Format(tstate, PyExc_TypeError,
                              "'async for' requires an object with "
                              "__aiter__ method, got %.100s",
                              type->tp_name);
                Py_DECREF(obj);
                goto error;
            }

            if (Py_TYPE(iter)->tp_as_async == NULL ||
                    Py_TYPE(iter)->tp_as_async->am_anext == NULL) {

                SET_TOP(NULL);
                _PyErr_Format(tstate, PyExc_TypeError,
                              "'async for' received an object from __aiter__ "
                              "that does not implement __anext__: %.100s",
                              Py_TYPE(iter)->tp_name);
                Py_DECREF(iter);
                goto error;
            }

            SET_TOP(iter);
        }

        // stack effect: ( -- __0)
        inst(GET_ANEXT) {
            unaryfunc getter = NULL;
            PyObject *next_iter = NULL;
            PyObject *awaitable = NULL;
            PyObject *aiter = TOP();
            PyTypeObject *type = Py_TYPE(aiter);

            if (PyAsyncGen_CheckExact(aiter)) {
                awaitable = type->tp_as_async->am_anext(aiter);
                if (awaitable == NULL) {
                    goto error;
                }
            } else {
                if (type->tp_as_async != NULL){
                    getter = type->tp_as_async->am_anext;
                }

                if (getter != NULL) {
                    next_iter = (*getter)(aiter);
                    if (next_iter == NULL) {
                        goto error;
                    }
                }
                else {
                    _PyErr_Format(tstate, PyExc_TypeError,
                                  "'async for' requires an iterator with "
                                  "__anext__ method, got %.100s",
                                  type->tp_name);
                    goto error;
                }

                awaitable = _PyCoro_GetAwaitableIter(next_iter);
                if (awaitable == NULL) {
                    _PyErr_FormatFromCause(
                        PyExc_TypeError,
                        "'async for' received an invalid object "
                        "from __anext__: %.100s",
                        Py_TYPE(next_iter)->tp_name);

                    Py_DECREF(next_iter);
                    goto error;
                } else {
                    Py_DECREF(next_iter);
                }
            }

            PUSH(awaitable);
            PREDICT(LOAD_CONST);
        }

        // stack effect: ( -- )
        inst(GET_AWAITABLE) {
            PyObject *iterable = TOP();
            PyObject *iter = _PyCoro_GetAwaitableIter(iterable);

            if (iter == NULL) {
                format_awaitable_error(tstate, Py_TYPE(iterable), oparg);
            }

            Py_DECREF(iterable);

            if (iter != NULL && PyCoro_CheckExact(iter)) {
                PyObject *yf = _PyGen_yf((PyGenObject*)iter);
                if (yf != NULL) {
                    /* `iter` is a coroutine object that is being
                       awaited, `yf` is a pointer to the current awaitable
                       being awaited on. */
                    Py_DECREF(yf);
                    Py_CLEAR(iter);
                    _PyErr_SetString(tstate, PyExc_RuntimeError,
                                     "coroutine is being awaited already");
                    /* The code below jumps to `error` if `iter` is NULL. */
                }
            }

            SET_TOP(iter); /* Even if it's NULL */

            if (iter == NULL) {
                goto error;
            }

            PREDICT(LOAD_CONST);
        }

        // error: SEND stack effect depends on jump flag
        inst(SEND) {
            assert(STACK_LEVEL() >= 2);
            PyObject *v = POP();
            PyObject *receiver = TOP();
            PySendResult gen_status;
            PyObject *retval;
            if (tstate->c_tracefunc == NULL) {
                gen_status = PyIter_Send(receiver, v, &retval);
            } else {
                if (Py_IsNone(v) && PyIter_Check(receiver)) {
                    retval = Py_TYPE(receiver)->tp_iternext(receiver);
                }
                else {
                    retval = PyObject_CallMethodOneArg(receiver, &_Py_ID(send), v);
                }
                if (retval == NULL) {
                    if (tstate->c_tracefunc != NULL
                            && _PyErr_ExceptionMatches(tstate, PyExc_StopIteration))
                        call_exc_trace(tstate->c_tracefunc, tstate->c_traceobj, tstate, frame);
                    if (_PyGen_FetchStopIterationValue(&retval) == 0) {
                        gen_status = PYGEN_RETURN;
                    }
                    else {
                        gen_status = PYGEN_ERROR;
                    }
                }
                else {
                    gen_status = PYGEN_NEXT;
                }
            }
            Py_DECREF(v);
            if (gen_status == PYGEN_ERROR) {
                assert(retval == NULL);
                goto error;
            }
            if (gen_status == PYGEN_RETURN) {
                assert(retval != NULL);
                Py_DECREF(receiver);
                SET_TOP(retval);
                JUMPBY(oparg);
                DISPATCH();
            }
            assert(gen_status == PYGEN_NEXT);
            assert(retval != NULL);
            PUSH(retval);
        }

        // stack effect: ( -- )
        inst(ASYNC_GEN_WRAP) {
            PyObject *v = TOP();
            assert(frame->f_code->co_flags & CO_ASYNC_GENERATOR);
            PyObject *w = _PyAsyncGenValueWrapperNew(v);
            if (w == NULL) {
                goto error;
            }
            SET_TOP(w);
            Py_DECREF(v);
        }

        // stack effect: ( -- )
        inst(YIELD_VALUE) {
            // NOTE: It's important that YIELD_VALUE never raises an exception!
            // The compiler treats any exception raised here as a failed close()
            // or throw() call.
            assert(oparg == STACK_LEVEL());
            PyObject *retval = POP();
            PyGenObject *gen = _PyFrame_GetGenerator(frame);
            gen->gi_frame_state = FRAME_SUSPENDED;
            _PyFrame_SetStackPointer(frame, stack_pointer);
            TRACE_FUNCTION_EXIT();
            DTRACE_FUNCTION_EXIT();
            tstate->exc_info = gen->gi_exc_state.previous_item;
            gen->gi_exc_state.previous_item = NULL;
            _Py_LeaveRecursiveCallPy(tstate);
            if (!frame->is_entry) {
                frame = cframe.current_frame = frame->previous;
                frame->prev_instr -= frame->yield_offset;
                _PyFrame_StackPush(frame, retval);
                goto resume_frame;
            }
            _Py_LeaveRecursiveCallTstate(tstate);
            /* Restore previous cframe and return. */
            tstate->cframe = cframe.previous;
            tstate->cframe->use_tracing = cframe.use_tracing;
            assert(tstate->cframe->current_frame == frame->previous);
            assert(!_PyErr_Occurred(tstate));
            return retval;
        }

        // stack effect: (__0 -- )
        inst(POP_EXCEPT) {
            _PyErr_StackItem *exc_info = tstate->exc_info;
            PyObject *value = exc_info->exc_value;
            exc_info->exc_value = POP();
            Py_XDECREF(value);
        }

        // stack effect: (__0 -- )
        inst(RERAISE) {
            if (oparg) {
                PyObject *lasti = PEEK(oparg + 1);
                if (PyLong_Check(lasti)) {
                    frame->prev_instr = first_instr + PyLong_AsLong(lasti);
                    assert(!_PyErr_Occurred(tstate));
                }
                else {
                    assert(PyLong_Check(lasti));
                    _PyErr_SetString(tstate, PyExc_SystemError, "lasti is not an int");
                    goto error;
                }
            }
            PyObject *val = POP();
            assert(val && PyExceptionInstance_Check(val));
            PyObject *exc = Py_NewRef(PyExceptionInstance_Class(val));
            PyObject *tb = PyException_GetTraceback(val);
            _PyErr_Restore(tstate, exc, val, tb);
            goto exception_unwind;
        }

        // stack effect: (__0 -- )
        inst(PREP_RERAISE_STAR) {
            PyObject *excs = POP();
            assert(PyList_Check(excs));
            PyObject *orig = POP();

            PyObject *val = _PyExc_PrepReraiseStar(orig, excs);
            Py_DECREF(excs);
            Py_DECREF(orig);

            if (val == NULL) {
                goto error;
            }

            PUSH(val);
        }

        // stack effect: (__0, __1 -- )
        inst(END_ASYNC_FOR) {
            PyObject *val = POP();
            assert(val && PyExceptionInstance_Check(val));
            if (PyErr_GivenExceptionMatches(val, PyExc_StopAsyncIteration)) {
                Py_DECREF(val);
                Py_DECREF(POP());
                DISPATCH();
            }
            else {
                PyObject *exc = Py_NewRef(PyExceptionInstance_Class(val));
                PyObject *tb = PyException_GetTraceback(val);
                _PyErr_Restore(tstate, exc, val, tb);
                goto exception_unwind;
            }
        }

        // stack effect: (__0, __1 -- )
        inst(CLEANUP_THROW) {
            assert(throwflag);
            PyObject *exc_value = TOP();
            assert(exc_value && PyExceptionInstance_Check(exc_value));
            if (PyErr_GivenExceptionMatches(exc_value, PyExc_StopIteration)) {
                PyObject *value = ((PyStopIterationObject *)exc_value)->value;
                Py_INCREF(value);
                Py_DECREF(POP());  // The StopIteration.
                Py_DECREF(POP());  // The last sent value.
                Py_DECREF(POP());  // The delegated sub-iterator.
                PUSH(value);
                DISPATCH();
            }
            Py_INCREF(exc_value);
            PyObject *exc_type = Py_NewRef(Py_TYPE(exc_value));
            PyObject *exc_traceback = PyException_GetTraceback(exc_value);
            _PyErr_Restore(tstate, exc_type, exc_value, exc_traceback);
            goto exception_unwind;
        }

        inst(STOPITERATION_ERROR) {
            assert(frame->owner == FRAME_OWNED_BY_GENERATOR);
            PyObject *exc = TOP();
            assert(PyExceptionInstance_Check(exc));
            const char *msg = NULL;
            if (PyErr_GivenExceptionMatches(exc, PyExc_StopIteration)) {
                msg = "generator raised StopIteration";
                if (frame->f_code->co_flags & CO_ASYNC_GENERATOR) {
                    msg = "async generator raised StopIteration";
                }
                else if (frame->f_code->co_flags & CO_COROUTINE) {
                    msg = "coroutine raised StopIteration";
                }
            }
            else if ((frame->f_code->co_flags & CO_ASYNC_GENERATOR) &&
                    PyErr_GivenExceptionMatches(exc, PyExc_StopAsyncIteration))
            {
                /* code in `gen` raised a StopAsyncIteration error:
                raise a RuntimeError.
                */
                msg = "async generator raised StopAsyncIteration";
            }
            if (msg != NULL) {
                PyObject *message = _PyUnicode_FromASCII(msg, strlen(msg));
                if (message == NULL) {
                    goto error;
                }
                PyObject *error = PyObject_CallOneArg(PyExc_RuntimeError, message);
                if (error == NULL) {
                    Py_DECREF(message);
                    goto error;
                }
                assert(PyExceptionInstance_Check(error));
                SET_TOP(error);
                PyException_SetCause(error, exc);
                Py_INCREF(exc);
                PyException_SetContext(error, exc);
                Py_DECREF(message);
            }
            DISPATCH();
        }


        // stack effect: ( -- __0)
        inst(LOAD_ASSERTION_ERROR) {
            PyObject *value = PyExc_AssertionError;
            Py_INCREF(value);
            PUSH(value);
        }

        // stack effect: ( -- __0)
        inst(LOAD_BUILD_CLASS) {
            PyObject *bc;
            if (PyDict_CheckExact(BUILTINS())) {
                bc = _PyDict_GetItemWithError(BUILTINS(),
                                              &_Py_ID(__build_class__));
                if (bc == NULL) {
                    if (!_PyErr_Occurred(tstate)) {
                        _PyErr_SetString(tstate, PyExc_NameError,
                                         "__build_class__ not found");
                    }
                    goto error;
                }
                Py_INCREF(bc);
            }
            else {
                bc = PyObject_GetItem(BUILTINS(), &_Py_ID(__build_class__));
                if (bc == NULL) {
                    if (_PyErr_ExceptionMatches(tstate, PyExc_KeyError))
                        _PyErr_SetString(tstate, PyExc_NameError,
                                         "__build_class__ not found");
                    goto error;
                }
            }
            PUSH(bc);
        }

        // stack effect: (__0 -- )
        inst(STORE_NAME) {
            PyObject *name = GETITEM(names, oparg);
            PyObject *v = POP();
            PyObject *ns = LOCALS();
            int err;
            if (ns == NULL) {
                _PyErr_Format(tstate, PyExc_SystemError,
                              "no locals found when storing %R", name);
                Py_DECREF(v);
                goto error;
            }
            if (PyDict_CheckExact(ns))
                err = PyDict_SetItem(ns, name, v);
            else
                err = PyObject_SetItem(ns, name, v);
            Py_DECREF(v);
            if (err != 0)
                goto error;
        }

        // stack effect: ( -- )
        inst(DELETE_NAME) {
            PyObject *name = GETITEM(names, oparg);
            PyObject *ns = LOCALS();
            int err;
            if (ns == NULL) {
                _PyErr_Format(tstate, PyExc_SystemError,
                              "no locals when deleting %R", name);
                goto error;
            }
            err = PyObject_DelItem(ns, name);
            if (err != 0) {
                format_exc_check_arg(tstate, PyExc_NameError,
                                     NAME_ERROR_MSG,
                                     name);
                goto error;
            }
        }

        // stack effect: (__0 -- __array[oparg])
        inst(UNPACK_SEQUENCE) {
            PyObject *seq = POP();
            PyObject **top = stack_pointer + oparg;
            if (!unpack_iterable(tstate, seq, oparg, -1, top)) {
                Py_DECREF(seq);
                goto error;
            }
            STACK_GROW(oparg);
            Py_DECREF(seq);
            JUMPBY(INLINE_CACHE_ENTRIES_UNPACK_SEQUENCE);
        }

        // stack effect: (__0 -- __array[oparg])
        inst(UNPACK_SEQUENCE_ADAPTIVE) {
            assert(cframe.use_tracing == 0);
            _PyUnpackSequenceCache *cache = (_PyUnpackSequenceCache *)next_instr;
            if (ADAPTIVE_COUNTER_IS_ZERO(cache)) {
                PyObject *seq = TOP();
                next_instr--;
                _Py_Specialize_UnpackSequence(seq, next_instr, oparg);
                DISPATCH_SAME_OPARG();
            }
            else {
                STAT_INC(UNPACK_SEQUENCE, deferred);
                DECREMENT_ADAPTIVE_COUNTER(cache);
                GO_TO_INSTRUCTION(UNPACK_SEQUENCE);
            }
        }

        // stack effect: (__0 -- __array[oparg])
        inst(UNPACK_SEQUENCE_TWO_TUPLE) {
            PyObject *seq = TOP();
            DEOPT_IF(!PyTuple_CheckExact(seq), UNPACK_SEQUENCE);
            DEOPT_IF(PyTuple_GET_SIZE(seq) != 2, UNPACK_SEQUENCE);
            STAT_INC(UNPACK_SEQUENCE, hit);
            SET_TOP(Py_NewRef(PyTuple_GET_ITEM(seq, 1)));
            PUSH(Py_NewRef(PyTuple_GET_ITEM(seq, 0)));
            Py_DECREF(seq);
            JUMPBY(INLINE_CACHE_ENTRIES_UNPACK_SEQUENCE);
        }

        // stack effect: (__0 -- __array[oparg])
        inst(UNPACK_SEQUENCE_TUPLE) {
            PyObject *seq = TOP();
            DEOPT_IF(!PyTuple_CheckExact(seq), UNPACK_SEQUENCE);
            DEOPT_IF(PyTuple_GET_SIZE(seq) != oparg, UNPACK_SEQUENCE);
            STAT_INC(UNPACK_SEQUENCE, hit);
            STACK_SHRINK(1);
            PyObject **items = _PyTuple_ITEMS(seq);
            while (oparg--) {
                PUSH(Py_NewRef(items[oparg]));
            }
            Py_DECREF(seq);
            JUMPBY(INLINE_CACHE_ENTRIES_UNPACK_SEQUENCE);
        }

        // stack effect: (__0 -- __array[oparg])
        inst(UNPACK_SEQUENCE_LIST) {
            PyObject *seq = TOP();
            DEOPT_IF(!PyList_CheckExact(seq), UNPACK_SEQUENCE);
            DEOPT_IF(PyList_GET_SIZE(seq) != oparg, UNPACK_SEQUENCE);
            STAT_INC(UNPACK_SEQUENCE, hit);
            STACK_SHRINK(1);
            PyObject **items = _PyList_ITEMS(seq);
            while (oparg--) {
                PUSH(Py_NewRef(items[oparg]));
            }
            Py_DECREF(seq);
            JUMPBY(INLINE_CACHE_ENTRIES_UNPACK_SEQUENCE);
        }

        // error: UNPACK_EX has irregular stack effect
        inst(UNPACK_EX) {
            int totalargs = 1 + (oparg & 0xFF) + (oparg >> 8);
            PyObject *seq = POP();
            PyObject **top = stack_pointer + totalargs;
            if (!unpack_iterable(tstate, seq, oparg & 0xFF, oparg >> 8, top)) {
                Py_DECREF(seq);
                goto error;
            }
            STACK_GROW(totalargs);
            Py_DECREF(seq);
        }

        // stack effect: (__0, __1 -- )
        inst(STORE_ATTR) {
            PyObject *name = GETITEM(names, oparg);
            PyObject *owner = TOP();
            PyObject *v = SECOND();
            int err;
            STACK_SHRINK(2);
            err = PyObject_SetAttr(owner, name, v);
            Py_DECREF(v);
            Py_DECREF(owner);
            if (err != 0) {
                goto error;
            }
            JUMPBY(INLINE_CACHE_ENTRIES_STORE_ATTR);
        }

        // stack effect: (__0 -- )
        inst(DELETE_ATTR) {
            PyObject *name = GETITEM(names, oparg);
            PyObject *owner = POP();
            int err;
            err = PyObject_SetAttr(owner, name, (PyObject *)NULL);
            Py_DECREF(owner);
            if (err != 0)
                goto error;
        }

        // stack effect: (__0 -- )
        inst(STORE_GLOBAL) {
            PyObject *name = GETITEM(names, oparg);
            PyObject *v = POP();
            int err;
            err = PyDict_SetItem(GLOBALS(), name, v);
            Py_DECREF(v);
            if (err != 0)
                goto error;
        }

        // stack effect: ( -- )
        inst(DELETE_GLOBAL) {
            PyObject *name = GETITEM(names, oparg);
            int err;
            err = PyDict_DelItem(GLOBALS(), name);
            if (err != 0) {
                if (_PyErr_ExceptionMatches(tstate, PyExc_KeyError)) {
                    format_exc_check_arg(tstate, PyExc_NameError,
                                         NAME_ERROR_MSG, name);
                }
                goto error;
            }
        }

        // stack effect: ( -- __0)
        inst(LOAD_NAME) {
            PyObject *name = GETITEM(names, oparg);
            PyObject *locals = LOCALS();
            PyObject *v;
            if (locals == NULL) {
                _PyErr_Format(tstate, PyExc_SystemError,
                              "no locals when loading %R", name);
                goto error;
            }
            if (PyDict_CheckExact(locals)) {
                v = PyDict_GetItemWithError(locals, name);
                if (v != NULL) {
                    Py_INCREF(v);
                }
                else if (_PyErr_Occurred(tstate)) {
                    goto error;
                }
            }
            else {
                v = PyObject_GetItem(locals, name);
                if (v == NULL) {
                    if (!_PyErr_ExceptionMatches(tstate, PyExc_KeyError))
                        goto error;
                    _PyErr_Clear(tstate);
                }
            }
            if (v == NULL) {
                v = PyDict_GetItemWithError(GLOBALS(), name);
                if (v != NULL) {
                    Py_INCREF(v);
                }
                else if (_PyErr_Occurred(tstate)) {
                    goto error;
                }
                else {
                    if (PyDict_CheckExact(BUILTINS())) {
                        v = PyDict_GetItemWithError(BUILTINS(), name);
                        if (v == NULL) {
                            if (!_PyErr_Occurred(tstate)) {
                                format_exc_check_arg(
                                        tstate, PyExc_NameError,
                                        NAME_ERROR_MSG, name);
                            }
                            goto error;
                        }
                        Py_INCREF(v);
                    }
                    else {
                        v = PyObject_GetItem(BUILTINS(), name);
                        if (v == NULL) {
                            if (_PyErr_ExceptionMatches(tstate, PyExc_KeyError)) {
                                format_exc_check_arg(
                                            tstate, PyExc_NameError,
                                            NAME_ERROR_MSG, name);
                            }
                            goto error;
                        }
                    }
                }
            }
            PUSH(v);
        }

        // error: LOAD_GLOBAL has irregular stack effect
        inst(LOAD_GLOBAL) {
            int push_null = oparg & 1;
            PEEK(0) = NULL;
            PyObject *name = GETITEM(names, oparg>>1);
            PyObject *v;
            if (PyDict_CheckExact(GLOBALS())
                && PyDict_CheckExact(BUILTINS()))
            {
                v = _PyDict_LoadGlobal((PyDictObject *)GLOBALS(),
                                       (PyDictObject *)BUILTINS(),
                                       name);
                if (v == NULL) {
                    if (!_PyErr_Occurred(tstate)) {
                        /* _PyDict_LoadGlobal() returns NULL without raising
                         * an exception if the key doesn't exist */
                        format_exc_check_arg(tstate, PyExc_NameError,
                                             NAME_ERROR_MSG, name);
                    }
                    goto error;
                }
                Py_INCREF(v);
            }
            else {
                /* Slow-path if globals or builtins is not a dict */

                /* namespace 1: globals */
                v = PyObject_GetItem(GLOBALS(), name);
                if (v == NULL) {
                    if (!_PyErr_ExceptionMatches(tstate, PyExc_KeyError)) {
                        goto error;
                    }
                    _PyErr_Clear(tstate);

                    /* namespace 2: builtins */
                    v = PyObject_GetItem(BUILTINS(), name);
                    if (v == NULL) {
                        if (_PyErr_ExceptionMatches(tstate, PyExc_KeyError)) {
                            format_exc_check_arg(
                                        tstate, PyExc_NameError,
                                        NAME_ERROR_MSG, name);
                        }
                        goto error;
                    }
                }
            }
            /* Skip over inline cache */
            JUMPBY(INLINE_CACHE_ENTRIES_LOAD_GLOBAL);
            STACK_GROW(push_null);
            PUSH(v);
        }

        // error: LOAD_GLOBAL has irregular stack effect
        inst(LOAD_GLOBAL_ADAPTIVE) {
            assert(cframe.use_tracing == 0);
            _PyLoadGlobalCache *cache = (_PyLoadGlobalCache *)next_instr;
            if (ADAPTIVE_COUNTER_IS_ZERO(cache)) {
                PyObject *name = GETITEM(names, oparg>>1);
                next_instr--;
                if (_Py_Specialize_LoadGlobal(GLOBALS(), BUILTINS(), next_instr, name) < 0) {
                    goto error;
                }
                DISPATCH_SAME_OPARG();
            }
            else {
                STAT_INC(LOAD_GLOBAL, deferred);
                DECREMENT_ADAPTIVE_COUNTER(cache);
                GO_TO_INSTRUCTION(LOAD_GLOBAL);
            }
        }

        // error: LOAD_GLOBAL has irregular stack effect
        inst(LOAD_GLOBAL_MODULE) {
            assert(cframe.use_tracing == 0);
            DEOPT_IF(!PyDict_CheckExact(GLOBALS()), LOAD_GLOBAL);
            PyDictObject *dict = (PyDictObject *)GLOBALS();
            _PyLoadGlobalCache *cache = (_PyLoadGlobalCache *)next_instr;
            uint32_t version = read_u32(cache->module_keys_version);
            DEOPT_IF(dict->ma_keys->dk_version != version, LOAD_GLOBAL);
            assert(DK_IS_UNICODE(dict->ma_keys));
            PyDictUnicodeEntry *entries = DK_UNICODE_ENTRIES(dict->ma_keys);
            PyObject *res = entries[cache->index].me_value;
            DEOPT_IF(res == NULL, LOAD_GLOBAL);
            int push_null = oparg & 1;
            PEEK(0) = NULL;
            JUMPBY(INLINE_CACHE_ENTRIES_LOAD_GLOBAL);
            STAT_INC(LOAD_GLOBAL, hit);
            STACK_GROW(push_null+1);
            Py_INCREF(res);
            SET_TOP(res);
        }

        // error: LOAD_GLOBAL has irregular stack effect
        inst(LOAD_GLOBAL_BUILTIN) {
            assert(cframe.use_tracing == 0);
            DEOPT_IF(!PyDict_CheckExact(GLOBALS()), LOAD_GLOBAL);
            DEOPT_IF(!PyDict_CheckExact(BUILTINS()), LOAD_GLOBAL);
            PyDictObject *mdict = (PyDictObject *)GLOBALS();
            PyDictObject *bdict = (PyDictObject *)BUILTINS();
            _PyLoadGlobalCache *cache = (_PyLoadGlobalCache *)next_instr;
            uint32_t mod_version = read_u32(cache->module_keys_version);
            uint16_t bltn_version = cache->builtin_keys_version;
            DEOPT_IF(mdict->ma_keys->dk_version != mod_version, LOAD_GLOBAL);
            DEOPT_IF(bdict->ma_keys->dk_version != bltn_version, LOAD_GLOBAL);
            assert(DK_IS_UNICODE(bdict->ma_keys));
            PyDictUnicodeEntry *entries = DK_UNICODE_ENTRIES(bdict->ma_keys);
            PyObject *res = entries[cache->index].me_value;
            DEOPT_IF(res == NULL, LOAD_GLOBAL);
            int push_null = oparg & 1;
            PEEK(0) = NULL;
            JUMPBY(INLINE_CACHE_ENTRIES_LOAD_GLOBAL);
            STAT_INC(LOAD_GLOBAL, hit);
            STACK_GROW(push_null+1);
            Py_INCREF(res);
            SET_TOP(res);
        }

        // stack effect: ( -- )
        inst(DELETE_FAST) {
            PyObject *v = GETLOCAL(oparg);
            if (v != NULL) {
                SETLOCAL(oparg, NULL);
                DISPATCH();
            }
            goto unbound_local_error;
        }

        // stack effect: ( -- )
        inst(MAKE_CELL) {
            // "initial" is probably NULL but not if it's an arg (or set
            // via PyFrame_LocalsToFast() before MAKE_CELL has run).
            PyObject *initial = GETLOCAL(oparg);
            PyObject *cell = PyCell_New(initial);
            if (cell == NULL) {
                goto resume_with_error;
            }
            SETLOCAL(oparg, cell);
        }

        // stack effect: ( -- )
        inst(DELETE_DEREF) {
            PyObject *cell = GETLOCAL(oparg);
            PyObject *oldobj = PyCell_GET(cell);
            if (oldobj != NULL) {
                PyCell_SET(cell, NULL);
                Py_DECREF(oldobj);
                DISPATCH();
            }
            format_exc_unbound(tstate, frame->f_code, oparg);
            goto error;
        }

        // stack effect: ( -- __0)
        inst(LOAD_CLASSDEREF) {
            PyObject *name, *value, *locals = LOCALS();
            assert(locals);
            assert(oparg >= 0 && oparg < frame->f_code->co_nlocalsplus);
            name = PyTuple_GET_ITEM(frame->f_code->co_localsplusnames, oparg);
            if (PyDict_CheckExact(locals)) {
                value = PyDict_GetItemWithError(locals, name);
                if (value != NULL) {
                    Py_INCREF(value);
                }
                else if (_PyErr_Occurred(tstate)) {
                    goto error;
                }
            }
            else {
                value = PyObject_GetItem(locals, name);
                if (value == NULL) {
                    if (!_PyErr_ExceptionMatches(tstate, PyExc_KeyError)) {
                        goto error;
                    }
                    _PyErr_Clear(tstate);
                }
            }
            if (!value) {
                PyObject *cell = GETLOCAL(oparg);
                value = PyCell_GET(cell);
                if (value == NULL) {
                    format_exc_unbound(tstate, frame->f_code, oparg);
                    goto error;
                }
                Py_INCREF(value);
            }
            PUSH(value);
        }

        // stack effect: ( -- __0)
        inst(LOAD_DEREF) {
            PyObject *cell = GETLOCAL(oparg);
            PyObject *value = PyCell_GET(cell);
            if (value == NULL) {
                format_exc_unbound(tstate, frame->f_code, oparg);
                goto error;
            }
            Py_INCREF(value);
            PUSH(value);
        }

        // stack effect: (__0 -- )
        inst(STORE_DEREF) {
            PyObject *v = POP();
            PyObject *cell = GETLOCAL(oparg);
            PyObject *oldobj = PyCell_GET(cell);
            PyCell_SET(cell, v);
            Py_XDECREF(oldobj);
        }

        // stack effect: ( -- )
        inst(COPY_FREE_VARS) {
            /* Copy closure variables to free variables */
            PyCodeObject *co = frame->f_code;
            assert(PyFunction_Check(frame->f_funcobj));
            PyObject *closure = ((PyFunctionObject *)frame->f_funcobj)->func_closure;
            int offset = co->co_nlocals + co->co_nplaincellvars;
            assert(oparg == co->co_nfreevars);
            for (int i = 0; i < oparg; ++i) {
                PyObject *o = PyTuple_GET_ITEM(closure, i);
                Py_INCREF(o);
                frame->localsplus[offset + i] = o;
            }
        }

        // stack effect: (__array[oparg] -- __0)
        inst(BUILD_STRING) {
            PyObject *str;
            str = _PyUnicode_JoinArray(&_Py_STR(empty),
                                       stack_pointer - oparg, oparg);
            if (str == NULL)
                goto error;
            while (--oparg >= 0) {
                PyObject *item = POP();
                Py_DECREF(item);
            }
            PUSH(str);
        }

        // stack effect: (__array[oparg] -- __0)
        inst(BUILD_TUPLE) {
            STACK_SHRINK(oparg);
            PyObject *tup = _PyTuple_FromArraySteal(stack_pointer, oparg);
            if (tup == NULL)
                goto error;
            PUSH(tup);
        }

        // stack effect: (__array[oparg] -- __0)
        inst(BUILD_LIST) {
            PyObject *list =  PyList_New(oparg);
            if (list == NULL)
                goto error;
            while (--oparg >= 0) {
                PyObject *item = POP();
                PyList_SET_ITEM(list, oparg, item);
            }
            PUSH(list);
        }

        // stack effect: ( -- )
        inst(LIST_TO_TUPLE) {
            PyObject *list = POP();
            PyObject *tuple = PyList_AsTuple(list);
            Py_DECREF(list);
            if (tuple == NULL) {
                goto error;
            }
            PUSH(tuple);
        }

        // stack effect: (__0 -- )
        inst(LIST_EXTEND) {
            PyObject *iterable = POP();
            PyObject *list = PEEK(oparg);
            PyObject *none_val = _PyList_Extend((PyListObject *)list, iterable);
            if (none_val == NULL) {
                if (_PyErr_ExceptionMatches(tstate, PyExc_TypeError) &&
                   (Py_TYPE(iterable)->tp_iter == NULL && !PySequence_Check(iterable)))
                {
                    _PyErr_Clear(tstate);
                    _PyErr_Format(tstate, PyExc_TypeError,
                          "Value after * must be an iterable, not %.200s",
                          Py_TYPE(iterable)->tp_name);
                }
                Py_DECREF(iterable);
                goto error;
            }
            Py_DECREF(none_val);
            Py_DECREF(iterable);
        }

        // stack effect: (__0 -- )
        inst(SET_UPDATE) {
            PyObject *iterable = POP();
            PyObject *set = PEEK(oparg);
            int err = _PySet_Update(set, iterable);
            Py_DECREF(iterable);
            if (err < 0) {
                goto error;
            }
        }

        // stack effect: (__array[oparg] -- __0)
        inst(BUILD_SET) {
            PyObject *set = PySet_New(NULL);
            int err = 0;
            int i;
            if (set == NULL)
                goto error;
            for (i = oparg; i > 0; i--) {
                PyObject *item = PEEK(i);
                if (err == 0)
                    err = PySet_Add(set, item);
                Py_DECREF(item);
            }
            STACK_SHRINK(oparg);
            if (err != 0) {
                Py_DECREF(set);
                goto error;
            }
            PUSH(set);
        }

        // stack effect: (__array[oparg*2] -- __0)
        inst(BUILD_MAP) {
            PyObject *map = _PyDict_FromItems(
                    &PEEK(2*oparg), 2,
                    &PEEK(2*oparg - 1), 2,
                    oparg);
            if (map == NULL)
                goto error;

            while (oparg--) {
                Py_DECREF(POP());
                Py_DECREF(POP());
            }
            PUSH(map);
        }

        // stack effect: ( -- )
        inst(SETUP_ANNOTATIONS) {
            int err;
            PyObject *ann_dict;
            if (LOCALS() == NULL) {
                _PyErr_Format(tstate, PyExc_SystemError,
                              "no locals found when setting up annotations");
                goto error;
            }
            /* check if __annotations__ in locals()... */
            if (PyDict_CheckExact(LOCALS())) {
                ann_dict = _PyDict_GetItemWithError(LOCALS(),
                                                    &_Py_ID(__annotations__));
                if (ann_dict == NULL) {
                    if (_PyErr_Occurred(tstate)) {
                        goto error;
                    }
                    /* ...if not, create a new one */
                    ann_dict = PyDict_New();
                    if (ann_dict == NULL) {
                        goto error;
                    }
                    err = PyDict_SetItem(LOCALS(), &_Py_ID(__annotations__),
                                         ann_dict);
                    Py_DECREF(ann_dict);
                    if (err != 0) {
                        goto error;
                    }
                }
            }
            else {
                /* do the same if locals() is not a dict */
                ann_dict = PyObject_GetItem(LOCALS(), &_Py_ID(__annotations__));
                if (ann_dict == NULL) {
                    if (!_PyErr_ExceptionMatches(tstate, PyExc_KeyError)) {
                        goto error;
                    }
                    _PyErr_Clear(tstate);
                    ann_dict = PyDict_New();
                    if (ann_dict == NULL) {
                        goto error;
                    }
                    err = PyObject_SetItem(LOCALS(), &_Py_ID(__annotations__),
                                           ann_dict);
                    Py_DECREF(ann_dict);
                    if (err != 0) {
                        goto error;
                    }
                }
                else {
                    Py_DECREF(ann_dict);
                }
            }
        }

        // stack effect: (__array[oparg] -- )
        inst(BUILD_CONST_KEY_MAP) {
            PyObject *map;
            PyObject *keys = TOP();
            if (!PyTuple_CheckExact(keys) ||
                PyTuple_GET_SIZE(keys) != (Py_ssize_t)oparg) {
                _PyErr_SetString(tstate, PyExc_SystemError,
                                 "bad BUILD_CONST_KEY_MAP keys argument");
                goto error;
            }
            map = _PyDict_FromItems(
                    &PyTuple_GET_ITEM(keys, 0), 1,
                    &PEEK(oparg + 1), 1, oparg);
            if (map == NULL) {
                goto error;
            }

            Py_DECREF(POP());
            while (oparg--) {
                Py_DECREF(POP());
            }
            PUSH(map);
        }

        // stack effect: (__0 -- )
        inst(DICT_UPDATE) {
            PyObject *update = POP();
            PyObject *dict = PEEK(oparg);
            if (PyDict_Update(dict, update) < 0) {
                if (_PyErr_ExceptionMatches(tstate, PyExc_AttributeError)) {
                    _PyErr_Format(tstate, PyExc_TypeError,
                                    "'%.200s' object is not a mapping",
                                    Py_TYPE(update)->tp_name);
                }
                Py_DECREF(update);
                goto error;
            }
            Py_DECREF(update);
        }

        // stack effect: (__0 -- )
        inst(DICT_MERGE) {
            PyObject *update = POP();
            PyObject *dict = PEEK(oparg);

            if (_PyDict_MergeEx(dict, update, 2) < 0) {
                format_kwargs_error(tstate, PEEK(2 + oparg), update);
                Py_DECREF(update);
                goto error;
            }
            Py_DECREF(update);
            PREDICT(CALL_FUNCTION_EX);
        }

        // stack effect: (__0, __1 -- )
        inst(MAP_ADD) {
            PyObject *value = TOP();
            PyObject *key = SECOND();
            PyObject *map;
            STACK_SHRINK(2);
            map = PEEK(oparg);                      /* dict */
            assert(PyDict_CheckExact(map));
            /* map[key] = value */
            if (_PyDict_SetItem_Take2((PyDictObject *)map, key, value) != 0) {
                goto error;
            }
            PREDICT(JUMP_BACKWARD);
        }

        // error: LOAD_ATTR has irregular stack effect
        inst(LOAD_ATTR) {
            PyObject *name = GETITEM(names, oparg >> 1);
            PyObject *owner = TOP();
            if (oparg & 1) {
                /* Designed to work in tandem with CALL. */
                PyObject* meth = NULL;

                int meth_found = _PyObject_GetMethod(owner, name, &meth);

                if (meth == NULL) {
                    /* Most likely attribute wasn't found. */
                    goto error;
                }

                if (meth_found) {
                    /* We can bypass temporary bound method object.
                       meth is unbound method and obj is self.

                       meth | self | arg1 | ... | argN
                     */
                    SET_TOP(meth);
                    PUSH(owner);  // self
                }
                else {
                    /* meth is not an unbound method (but a regular attr, or
                       something was returned by a descriptor protocol).  Set
                       the second element of the stack to NULL, to signal
                       CALL that it's not a method call.

                       NULL | meth | arg1 | ... | argN
                    */
                    SET_TOP(NULL);
                    Py_DECREF(owner);
                    PUSH(meth);
                }
                JUMPBY(INLINE_CACHE_ENTRIES_LOAD_ATTR);
                DISPATCH();
            }
            PyObject *res = PyObject_GetAttr(owner, name);
            if (res == NULL) {
                goto error;
            }
            Py_DECREF(owner);
            SET_TOP(res);
            JUMPBY(INLINE_CACHE_ENTRIES_LOAD_ATTR);
        }

        // error: LOAD_ATTR has irregular stack effect
        inst(LOAD_ATTR_ADAPTIVE) {
            assert(cframe.use_tracing == 0);
            _PyAttrCache *cache = (_PyAttrCache *)next_instr;
            if (ADAPTIVE_COUNTER_IS_ZERO(cache)) {
                PyObject *owner = TOP();
                PyObject *name = GETITEM(names, oparg>>1);
                next_instr--;
                if (_Py_Specialize_LoadAttr(owner, next_instr, name) < 0) {
                    goto error;
                }
                DISPATCH_SAME_OPARG();
            }
            else {
                STAT_INC(LOAD_ATTR, deferred);
                DECREMENT_ADAPTIVE_COUNTER(cache);
                GO_TO_INSTRUCTION(LOAD_ATTR);
            }
        }

        // error: LOAD_ATTR has irregular stack effect
        inst(LOAD_ATTR_INSTANCE_VALUE) {
            assert(cframe.use_tracing == 0);
            PyObject *owner = TOP();
            PyObject *res;
            PyTypeObject *tp = Py_TYPE(owner);
            _PyAttrCache *cache = (_PyAttrCache *)next_instr;
            uint32_t type_version = read_u32(cache->version);
            assert(type_version != 0);
            DEOPT_IF(tp->tp_version_tag != type_version, LOAD_ATTR);
            assert(tp->tp_dictoffset < 0);
            assert(tp->tp_flags & Py_TPFLAGS_MANAGED_DICT);
            PyDictOrValues dorv = *_PyObject_DictOrValuesPointer(owner);
            DEOPT_IF(!_PyDictOrValues_IsValues(dorv), LOAD_ATTR);
            res = _PyDictOrValues_GetValues(dorv)->values[cache->index];
            DEOPT_IF(res == NULL, LOAD_ATTR);
            STAT_INC(LOAD_ATTR, hit);
            Py_INCREF(res);
            SET_TOP(NULL);
            STACK_GROW((oparg & 1));
            SET_TOP(res);
            Py_DECREF(owner);
            JUMPBY(INLINE_CACHE_ENTRIES_LOAD_ATTR);
        }

        // error: LOAD_ATTR has irregular stack effect
        inst(LOAD_ATTR_MODULE) {
            assert(cframe.use_tracing == 0);
            PyObject *owner = TOP();
            PyObject *res;
            _PyAttrCache *cache = (_PyAttrCache *)next_instr;
            DEOPT_IF(!PyModule_CheckExact(owner), LOAD_ATTR);
            PyDictObject *dict = (PyDictObject *)((PyModuleObject *)owner)->md_dict;
            assert(dict != NULL);
            DEOPT_IF(dict->ma_keys->dk_version != read_u32(cache->version),
                LOAD_ATTR);
            assert(dict->ma_keys->dk_kind == DICT_KEYS_UNICODE);
            assert(cache->index < dict->ma_keys->dk_nentries);
            PyDictUnicodeEntry *ep = DK_UNICODE_ENTRIES(dict->ma_keys) + cache->index;
            res = ep->me_value;
            DEOPT_IF(res == NULL, LOAD_ATTR);
            STAT_INC(LOAD_ATTR, hit);
            Py_INCREF(res);
            SET_TOP(NULL);
            STACK_GROW((oparg & 1));
            SET_TOP(res);
            Py_DECREF(owner);
            JUMPBY(INLINE_CACHE_ENTRIES_LOAD_ATTR);
        }

        // error: LOAD_ATTR has irregular stack effect
        inst(LOAD_ATTR_WITH_HINT) {
            assert(cframe.use_tracing == 0);
            PyObject *owner = TOP();
            PyObject *res;
            PyTypeObject *tp = Py_TYPE(owner);
            _PyAttrCache *cache = (_PyAttrCache *)next_instr;
            uint32_t type_version = read_u32(cache->version);
            assert(type_version != 0);
            DEOPT_IF(tp->tp_version_tag != type_version, LOAD_ATTR);
            assert(tp->tp_flags & Py_TPFLAGS_MANAGED_DICT);
            PyDictOrValues dorv = *_PyObject_DictOrValuesPointer(owner);
            DEOPT_IF(_PyDictOrValues_IsValues(dorv), LOAD_ATTR);
            PyDictObject *dict = (PyDictObject *)_PyDictOrValues_GetDict(dorv);
            DEOPT_IF(dict == NULL, LOAD_ATTR);
            assert(PyDict_CheckExact((PyObject *)dict));
            PyObject *name = GETITEM(names, oparg>>1);
            uint16_t hint = cache->index;
            DEOPT_IF(hint >= (size_t)dict->ma_keys->dk_nentries, LOAD_ATTR);
            if (DK_IS_UNICODE(dict->ma_keys)) {
                PyDictUnicodeEntry *ep = DK_UNICODE_ENTRIES(dict->ma_keys) + hint;
                DEOPT_IF(ep->me_key != name, LOAD_ATTR);
                res = ep->me_value;
            }
            else {
                PyDictKeyEntry *ep = DK_ENTRIES(dict->ma_keys) + hint;
                DEOPT_IF(ep->me_key != name, LOAD_ATTR);
                res = ep->me_value;
            }
            DEOPT_IF(res == NULL, LOAD_ATTR);
            STAT_INC(LOAD_ATTR, hit);
            Py_INCREF(res);
            SET_TOP(NULL);
            STACK_GROW((oparg & 1));
            SET_TOP(res);
            Py_DECREF(owner);
            JUMPBY(INLINE_CACHE_ENTRIES_LOAD_ATTR);
        }

        // error: LOAD_ATTR has irregular stack effect
        inst(LOAD_ATTR_SLOT) {
            assert(cframe.use_tracing == 0);
            PyObject *owner = TOP();
            PyObject *res;
            PyTypeObject *tp = Py_TYPE(owner);
            _PyAttrCache *cache = (_PyAttrCache *)next_instr;
            uint32_t type_version = read_u32(cache->version);
            assert(type_version != 0);
            DEOPT_IF(tp->tp_version_tag != type_version, LOAD_ATTR);
            char *addr = (char *)owner + cache->index;
            res = *(PyObject **)addr;
            DEOPT_IF(res == NULL, LOAD_ATTR);
            STAT_INC(LOAD_ATTR, hit);
            Py_INCREF(res);
            SET_TOP(NULL);
            STACK_GROW((oparg & 1));
            SET_TOP(res);
            Py_DECREF(owner);
            JUMPBY(INLINE_CACHE_ENTRIES_LOAD_ATTR);
        }

        // error: LOAD_ATTR has irregular stack effect
        inst(LOAD_ATTR_CLASS) {
            assert(cframe.use_tracing == 0);
            _PyLoadMethodCache *cache = (_PyLoadMethodCache *)next_instr;

            PyObject *cls = TOP();
            DEOPT_IF(!PyType_Check(cls), LOAD_ATTR);
            uint32_t type_version = read_u32(cache->type_version);
            DEOPT_IF(((PyTypeObject *)cls)->tp_version_tag != type_version,
                LOAD_ATTR);
            assert(type_version != 0);

            STAT_INC(LOAD_ATTR, hit);
            PyObject *res = read_obj(cache->descr);
            assert(res != NULL);
            Py_INCREF(res);
            SET_TOP(NULL);
            STACK_GROW((oparg & 1));
            SET_TOP(res);
            Py_DECREF(cls);
            JUMPBY(INLINE_CACHE_ENTRIES_LOAD_ATTR);
        }

        // error: LOAD_ATTR has irregular stack effect
        inst(LOAD_ATTR_PROPERTY) {
            assert(cframe.use_tracing == 0);
            DEOPT_IF(tstate->interp->eval_frame, LOAD_ATTR);
            _PyLoadMethodCache *cache = (_PyLoadMethodCache *)next_instr;

            PyObject *owner = TOP();
            PyTypeObject *cls = Py_TYPE(owner);
            uint32_t type_version = read_u32(cache->type_version);
            DEOPT_IF(cls->tp_version_tag != type_version, LOAD_ATTR);
            assert(type_version != 0);
            PyObject *fget = read_obj(cache->descr);
            assert(Py_IS_TYPE(fget, &PyFunction_Type));
            PyFunctionObject *f = (PyFunctionObject *)fget;
            uint32_t func_version = read_u32(cache->keys_version);
            assert(func_version != 0);
            DEOPT_IF(f->func_version != func_version, LOAD_ATTR);
            PyCodeObject *code = (PyCodeObject *)f->func_code;
            assert(code->co_argcount == 1);
            DEOPT_IF(!_PyThreadState_HasStackSpace(tstate, code->co_framesize), LOAD_ATTR);
            STAT_INC(LOAD_ATTR, hit);
            Py_INCREF(fget);
            _PyInterpreterFrame *new_frame = _PyFrame_PushUnchecked(tstate, f);
            SET_TOP(NULL);
            int shrink_stack = !(oparg & 1);
            STACK_SHRINK(shrink_stack);
            new_frame->localsplus[0] = owner;
            for (int i = 1; i < code->co_nlocalsplus; i++) {
                new_frame->localsplus[i] = NULL;
            }
            _PyFrame_SetStackPointer(frame, stack_pointer);
            JUMPBY(INLINE_CACHE_ENTRIES_LOAD_ATTR);
            frame->prev_instr = next_instr - 1;
            new_frame->previous = frame;
            frame = cframe.current_frame = new_frame;
            CALL_STAT_INC(inlined_py_calls);
            goto start_frame;
        }

        // error: LOAD_ATTR has irregular stack effect
        inst(LOAD_ATTR_GETATTRIBUTE_OVERRIDDEN) {
            assert(cframe.use_tracing == 0);
            DEOPT_IF(tstate->interp->eval_frame, LOAD_ATTR);
            _PyLoadMethodCache *cache = (_PyLoadMethodCache *)next_instr;
            PyObject *owner = TOP();
            PyTypeObject *cls = Py_TYPE(owner);
            uint32_t type_version = read_u32(cache->type_version);
            DEOPT_IF(cls->tp_version_tag != type_version, LOAD_ATTR);
            assert(type_version != 0);
            PyObject *getattribute = read_obj(cache->descr);
            assert(Py_IS_TYPE(getattribute, &PyFunction_Type));
            PyFunctionObject *f = (PyFunctionObject *)getattribute;
            uint32_t func_version = read_u32(cache->keys_version);
            assert(func_version != 0);
            DEOPT_IF(f->func_version != func_version, LOAD_ATTR);
            PyCodeObject *code = (PyCodeObject *)f->func_code;
            assert(code->co_argcount == 2);
            DEOPT_IF(!_PyThreadState_HasStackSpace(tstate, code->co_framesize), CALL);
            STAT_INC(LOAD_ATTR, hit);

            PyObject *name = GETITEM(names, oparg >> 1);
            Py_INCREF(f);
            _PyInterpreterFrame *new_frame = _PyFrame_PushUnchecked(tstate, f);
            SET_TOP(NULL);
            int shrink_stack = !(oparg & 1);
            STACK_SHRINK(shrink_stack);
            Py_INCREF(name);
            new_frame->localsplus[0] = owner;
            new_frame->localsplus[1] = name;
            for (int i = 2; i < code->co_nlocalsplus; i++) {
                new_frame->localsplus[i] = NULL;
            }
            _PyFrame_SetStackPointer(frame, stack_pointer);
            JUMPBY(INLINE_CACHE_ENTRIES_LOAD_ATTR);
            frame->prev_instr = next_instr - 1;
            new_frame->previous = frame;
            frame = cframe.current_frame = new_frame;
            CALL_STAT_INC(inlined_py_calls);
            goto start_frame;
        }

        // stack effect: (__0, __1 -- )
        inst(STORE_ATTR_ADAPTIVE) {
            assert(cframe.use_tracing == 0);
            _PyAttrCache *cache = (_PyAttrCache *)next_instr;
            if (ADAPTIVE_COUNTER_IS_ZERO(cache)) {
                PyObject *owner = TOP();
                PyObject *name = GETITEM(names, oparg);
                next_instr--;
                if (_Py_Specialize_StoreAttr(owner, next_instr, name) < 0) {
                    goto error;
                }
                DISPATCH_SAME_OPARG();
            }
            else {
                STAT_INC(STORE_ATTR, deferred);
                DECREMENT_ADAPTIVE_COUNTER(cache);
                GO_TO_INSTRUCTION(STORE_ATTR);
            }
        }

        // stack effect: (__0, __1 -- )
        inst(STORE_ATTR_INSTANCE_VALUE) {
            assert(cframe.use_tracing == 0);
            PyObject *owner = TOP();
            PyTypeObject *tp = Py_TYPE(owner);
            _PyAttrCache *cache = (_PyAttrCache *)next_instr;
            uint32_t type_version = read_u32(cache->version);
            assert(type_version != 0);
            DEOPT_IF(tp->tp_version_tag != type_version, STORE_ATTR);
            assert(tp->tp_flags & Py_TPFLAGS_MANAGED_DICT);
            PyDictOrValues dorv = *_PyObject_DictOrValuesPointer(owner);
            DEOPT_IF(!_PyDictOrValues_IsValues(dorv), STORE_ATTR);
            STAT_INC(STORE_ATTR, hit);
            Py_ssize_t index = cache->index;
            STACK_SHRINK(1);
            PyObject *value = POP();
            PyDictValues *values = _PyDictOrValues_GetValues(dorv);
            PyObject *old_value = values->values[index];
            values->values[index] = value;
            if (old_value == NULL) {
                _PyDictValues_AddToInsertionOrder(values, index);
            }
            else {
                Py_DECREF(old_value);
            }
            Py_DECREF(owner);
            JUMPBY(INLINE_CACHE_ENTRIES_STORE_ATTR);
        }

        // stack effect: (__0, __1 -- )
        inst(STORE_ATTR_WITH_HINT) {
            assert(cframe.use_tracing == 0);
            PyObject *owner = TOP();
            PyTypeObject *tp = Py_TYPE(owner);
            _PyAttrCache *cache = (_PyAttrCache *)next_instr;
            uint32_t type_version = read_u32(cache->version);
            assert(type_version != 0);
            DEOPT_IF(tp->tp_version_tag != type_version, STORE_ATTR);
            assert(tp->tp_flags & Py_TPFLAGS_MANAGED_DICT);
            PyDictOrValues dorv = *_PyObject_DictOrValuesPointer(owner);
            DEOPT_IF(_PyDictOrValues_IsValues(dorv), LOAD_ATTR);
            PyDictObject *dict = (PyDictObject *)_PyDictOrValues_GetDict(dorv);
            DEOPT_IF(dict == NULL, STORE_ATTR);
            assert(PyDict_CheckExact((PyObject *)dict));
            PyObject *name = GETITEM(names, oparg);
            uint16_t hint = cache->index;
            DEOPT_IF(hint >= (size_t)dict->ma_keys->dk_nentries, STORE_ATTR);
            PyObject *value, *old_value;
            uint64_t new_version;
            if (DK_IS_UNICODE(dict->ma_keys)) {
                PyDictUnicodeEntry *ep = DK_UNICODE_ENTRIES(dict->ma_keys) + hint;
                DEOPT_IF(ep->me_key != name, STORE_ATTR);
                old_value = ep->me_value;
                DEOPT_IF(old_value == NULL, STORE_ATTR);
                STACK_SHRINK(1);
                value = POP();
                new_version = _PyDict_NotifyEvent(PyDict_EVENT_MODIFIED, dict, name, value);
                ep->me_value = value;
            }
            else {
                PyDictKeyEntry *ep = DK_ENTRIES(dict->ma_keys) + hint;
                DEOPT_IF(ep->me_key != name, STORE_ATTR);
                old_value = ep->me_value;
                DEOPT_IF(old_value == NULL, STORE_ATTR);
                STACK_SHRINK(1);
                value = POP();
                new_version = _PyDict_NotifyEvent(PyDict_EVENT_MODIFIED, dict, name, value);
                ep->me_value = value;
            }
            Py_DECREF(old_value);
            STAT_INC(STORE_ATTR, hit);
            /* Ensure dict is GC tracked if it needs to be */
            if (!_PyObject_GC_IS_TRACKED(dict) && _PyObject_GC_MAY_BE_TRACKED(value)) {
                _PyObject_GC_TRACK(dict);
            }
            /* PEP 509 */
            dict->ma_version_tag = new_version;
            Py_DECREF(owner);
            JUMPBY(INLINE_CACHE_ENTRIES_STORE_ATTR);
        }

        // stack effect: (__0, __1 -- )
        inst(STORE_ATTR_SLOT) {
            assert(cframe.use_tracing == 0);
            PyObject *owner = TOP();
            PyTypeObject *tp = Py_TYPE(owner);
            _PyAttrCache *cache = (_PyAttrCache *)next_instr;
            uint32_t type_version = read_u32(cache->version);
            assert(type_version != 0);
            DEOPT_IF(tp->tp_version_tag != type_version, STORE_ATTR);
            char *addr = (char *)owner + cache->index;
            STAT_INC(STORE_ATTR, hit);
            STACK_SHRINK(1);
            PyObject *value = POP();
            PyObject *old_value = *(PyObject **)addr;
            *(PyObject **)addr = value;
            Py_XDECREF(old_value);
            Py_DECREF(owner);
            JUMPBY(INLINE_CACHE_ENTRIES_STORE_ATTR);
        }

        // stack effect: (__0 -- )
        inst(COMPARE_OP) {
            assert(oparg <= Py_GE);
            PyObject *right = POP();
            PyObject *left = TOP();
            PyObject *res = PyObject_RichCompare(left, right, oparg);
            SET_TOP(res);
            Py_DECREF(left);
            Py_DECREF(right);
            if (res == NULL) {
                goto error;
            }
            JUMPBY(INLINE_CACHE_ENTRIES_COMPARE_OP);
        }

        // stack effect: (__0 -- )
        inst(COMPARE_OP_ADAPTIVE) {
            assert(cframe.use_tracing == 0);
            _PyCompareOpCache *cache = (_PyCompareOpCache *)next_instr;
            if (ADAPTIVE_COUNTER_IS_ZERO(cache)) {
                PyObject *right = TOP();
                PyObject *left = SECOND();
                next_instr--;
                _Py_Specialize_CompareOp(left, right, next_instr, oparg);
                DISPATCH_SAME_OPARG();
            }
            else {
                STAT_INC(COMPARE_OP, deferred);
                DECREMENT_ADAPTIVE_COUNTER(cache);
                GO_TO_INSTRUCTION(COMPARE_OP);
            }
        }

        // stack effect: (__0 -- )
        inst(COMPARE_OP_FLOAT_JUMP) {
            assert(cframe.use_tracing == 0);
            // Combined: COMPARE_OP (float ? float) + POP_JUMP_IF_(true/false)
            _PyCompareOpCache *cache = (_PyCompareOpCache *)next_instr;
            int when_to_jump_mask = cache->mask;
            PyObject *right = TOP();
            PyObject *left = SECOND();
            DEOPT_IF(!PyFloat_CheckExact(left), COMPARE_OP);
            DEOPT_IF(!PyFloat_CheckExact(right), COMPARE_OP);
            double dleft = PyFloat_AS_DOUBLE(left);
            double dright = PyFloat_AS_DOUBLE(right);
            int sign = (dleft > dright) - (dleft < dright);
            DEOPT_IF(isnan(dleft), COMPARE_OP);
            DEOPT_IF(isnan(dright), COMPARE_OP);
            STAT_INC(COMPARE_OP, hit);
            JUMPBY(INLINE_CACHE_ENTRIES_COMPARE_OP);
            NEXTOPARG();
            STACK_SHRINK(2);
            _Py_DECREF_SPECIALIZED(left, _PyFloat_ExactDealloc);
            _Py_DECREF_SPECIALIZED(right, _PyFloat_ExactDealloc);
            assert(opcode == POP_JUMP_IF_FALSE || opcode == POP_JUMP_IF_TRUE);
            int jump = (1 << (sign + 1)) & when_to_jump_mask;
            if (!jump) {
                next_instr++;
            }
            else {
                JUMPBY(1 + oparg);
            }
        }

        // stack effect: (__0 -- )
        inst(COMPARE_OP_INT_JUMP) {
            assert(cframe.use_tracing == 0);
            // Combined: COMPARE_OP (int ? int) + POP_JUMP_IF_(true/false)
            _PyCompareOpCache *cache = (_PyCompareOpCache *)next_instr;
            int when_to_jump_mask = cache->mask;
            PyObject *right = TOP();
            PyObject *left = SECOND();
            DEOPT_IF(!PyLong_CheckExact(left), COMPARE_OP);
            DEOPT_IF(!PyLong_CheckExact(right), COMPARE_OP);
            DEOPT_IF((size_t)(Py_SIZE(left) + 1) > 2, COMPARE_OP);
            DEOPT_IF((size_t)(Py_SIZE(right) + 1) > 2, COMPARE_OP);
            STAT_INC(COMPARE_OP, hit);
            assert(Py_ABS(Py_SIZE(left)) <= 1 && Py_ABS(Py_SIZE(right)) <= 1);
            Py_ssize_t ileft = Py_SIZE(left) * ((PyLongObject *)left)->ob_digit[0];
            Py_ssize_t iright = Py_SIZE(right) * ((PyLongObject *)right)->ob_digit[0];
            int sign = (ileft > iright) - (ileft < iright);
            JUMPBY(INLINE_CACHE_ENTRIES_COMPARE_OP);
            NEXTOPARG();
            STACK_SHRINK(2);
            _Py_DECREF_SPECIALIZED(left, (destructor)PyObject_Free);
            _Py_DECREF_SPECIALIZED(right, (destructor)PyObject_Free);
            assert(opcode == POP_JUMP_IF_FALSE || opcode == POP_JUMP_IF_TRUE);
            int jump = (1 << (sign + 1)) & when_to_jump_mask;
            if (!jump) {
                next_instr++;
            }
            else {
                JUMPBY(1 + oparg);
            }
        }

        // stack effect: (__0 -- )
        inst(COMPARE_OP_STR_JUMP) {
            assert(cframe.use_tracing == 0);
            // Combined: COMPARE_OP (str == str or str != str) + POP_JUMP_IF_(true/false)
            _PyCompareOpCache *cache = (_PyCompareOpCache *)next_instr;
            int invert = cache->mask;
            PyObject *right = TOP();
            PyObject *left = SECOND();
            DEOPT_IF(!PyUnicode_CheckExact(left), COMPARE_OP);
            DEOPT_IF(!PyUnicode_CheckExact(right), COMPARE_OP);
            STAT_INC(COMPARE_OP, hit);
            int res = _PyUnicode_Equal(left, right);
            assert(oparg == Py_EQ || oparg == Py_NE);
            JUMPBY(INLINE_CACHE_ENTRIES_COMPARE_OP);
            NEXTOPARG();
            assert(opcode == POP_JUMP_IF_FALSE || opcode == POP_JUMP_IF_TRUE);
            STACK_SHRINK(2);
            _Py_DECREF_SPECIALIZED(left, _PyUnicode_ExactDealloc);
            _Py_DECREF_SPECIALIZED(right, _PyUnicode_ExactDealloc);
            assert(res == 0 || res == 1);
            assert(invert == 0 || invert == 1);
            int jump = res ^ invert;
            if (!jump) {
                next_instr++;
            }
            else {
                JUMPBY(1 + oparg);
            }
        }

        // stack effect: (__0 -- )
        inst(IS_OP) {
            PyObject *right = POP();
            PyObject *left = TOP();
            int res = Py_Is(left, right) ^ oparg;
            PyObject *b = res ? Py_True : Py_False;
            Py_INCREF(b);
            SET_TOP(b);
            Py_DECREF(left);
            Py_DECREF(right);
        }

        // stack effect: (__0 -- )
        inst(CONTAINS_OP) {
            PyObject *right = POP();
            PyObject *left = POP();
            int res = PySequence_Contains(right, left);
            Py_DECREF(left);
            Py_DECREF(right);
            if (res < 0) {
                goto error;
            }
            PyObject *b = (res^oparg) ? Py_True : Py_False;
            Py_INCREF(b);
            PUSH(b);
        }

        // stack effect: ( -- )
        inst(CHECK_EG_MATCH) {
            PyObject *match_type = POP();
            if (check_except_star_type_valid(tstate, match_type) < 0) {
                Py_DECREF(match_type);
                goto error;
            }

            PyObject *exc_value = TOP();
            PyObject *match = NULL, *rest = NULL;
            int res = exception_group_match(exc_value, match_type,
                                            &match, &rest);
            Py_DECREF(match_type);
            if (res < 0) {
                goto error;
            }

            if (match == NULL || rest == NULL) {
                assert(match == NULL);
                assert(rest == NULL);
                goto error;
            }
            if (Py_IsNone(match)) {
                PUSH(match);
                Py_XDECREF(rest);
            }
            else {
                /* Total or partial match - update the stack from
                 * [val]
                 * to
                 * [rest, match]
                 * (rest can be Py_None)
                 */

                SET_TOP(rest);
                PUSH(match);
                PyErr_SetExcInfo(NULL, Py_NewRef(match), NULL);
                Py_DECREF(exc_value);
            }
        }

        // stack effect: ( -- )
        inst(CHECK_EXC_MATCH) {
            PyObject *right = POP();
            PyObject *left = TOP();
            assert(PyExceptionInstance_Check(left));
            if (check_except_type_valid(tstate, right) < 0) {
                 Py_DECREF(right);
                 goto error;
            }

            int res = PyErr_GivenExceptionMatches(left, right);
            Py_DECREF(right);
            PUSH(Py_NewRef(res ? Py_True : Py_False));
        }

        // stack effect: (__0 -- )
        inst(IMPORT_NAME) {
            PyObject *name = GETITEM(names, oparg);
            PyObject *fromlist = POP();
            PyObject *level = TOP();
            PyObject *res;
            res = import_name(tstate, frame, name, fromlist, level);
            Py_DECREF(level);
            Py_DECREF(fromlist);
            SET_TOP(res);
            if (res == NULL)
                goto error;
        }

        // stack effect: (__0 -- )
        inst(IMPORT_STAR) {
            PyObject *from = POP(), *locals;
            int err;
            if (_PyFrame_FastToLocalsWithError(frame) < 0) {
                Py_DECREF(from);
                goto error;
            }

            locals = LOCALS();
            if (locals == NULL) {
                _PyErr_SetString(tstate, PyExc_SystemError,
                                 "no locals found during 'import *'");
                Py_DECREF(from);
                goto error;
            }
            err = import_all_from(tstate, locals, from);
            _PyFrame_LocalsToFast(frame, 0);
            Py_DECREF(from);
            if (err != 0)
                goto error;
        }

        // stack effect: ( -- __0)
        inst(IMPORT_FROM) {
            PyObject *name = GETITEM(names, oparg);
            PyObject *from = TOP();
            PyObject *res;
            res = import_from(tstate, from, name);
            PUSH(res);
            if (res == NULL)
                goto error;
        }

        // stack effect: ( -- )
        inst(JUMP_FORWARD) {
            JUMPBY(oparg);
        }

        // stack effect: ( -- )
        inst(JUMP_BACKWARD) {
            assert(oparg < INSTR_OFFSET());
            JUMPBY(-oparg);
            CHECK_EVAL_BREAKER();
        }

        // stack effect: (__0 -- )
        inst(POP_JUMP_IF_FALSE) {
            PyObject *cond = POP();
            if (Py_IsTrue(cond)) {
                _Py_DECREF_NO_DEALLOC(cond);
            }
            else if (Py_IsFalse(cond)) {
                _Py_DECREF_NO_DEALLOC(cond);
                JUMPBY(oparg);
            }
            else {
                int err = PyObject_IsTrue(cond);
                Py_DECREF(cond);
                if (err > 0)
                    ;
                else if (err == 0) {
                    JUMPBY(oparg);
                }
                else
                    goto error;
            }
        }

        // stack effect: (__0 -- )
        inst(POP_JUMP_IF_TRUE) {
            PyObject *cond = POP();
            if (Py_IsFalse(cond)) {
                _Py_DECREF_NO_DEALLOC(cond);
            }
            else if (Py_IsTrue(cond)) {
                _Py_DECREF_NO_DEALLOC(cond);
                JUMPBY(oparg);
            }
            else {
                int err = PyObject_IsTrue(cond);
                Py_DECREF(cond);
                if (err > 0) {
                    JUMPBY(oparg);
                }
                else if (err == 0)
                    ;
                else
                    goto error;
            }
        }

        // stack effect: (__0 -- )
        inst(POP_JUMP_IF_NOT_NONE) {
            PyObject *value = POP();
            if (!Py_IsNone(value)) {
                JUMPBY(oparg);
            }
            Py_DECREF(value);
        }

        // stack effect: (__0 -- )
        inst(POP_JUMP_IF_NONE) {
            PyObject *value = POP();
            if (Py_IsNone(value)) {
                _Py_DECREF_NO_DEALLOC(value);
                JUMPBY(oparg);
            }
            else {
                Py_DECREF(value);
            }
        }

        // error: JUMP_IF_FALSE_OR_POP stack effect depends on jump flag
        inst(JUMP_IF_FALSE_OR_POP) {
            PyObject *cond = TOP();
            int err;
            if (Py_IsTrue(cond)) {
                STACK_SHRINK(1);
                _Py_DECREF_NO_DEALLOC(cond);
                DISPATCH();
            }
            if (Py_IsFalse(cond)) {
                JUMPBY(oparg);
                DISPATCH();
            }
            err = PyObject_IsTrue(cond);
            if (err > 0) {
                STACK_SHRINK(1);
                Py_DECREF(cond);
            }
            else if (err == 0)
                JUMPBY(oparg);
            else
                goto error;
        }

        // error: JUMP_IF_TRUE_OR_POP stack effect depends on jump flag
        inst(JUMP_IF_TRUE_OR_POP) {
            PyObject *cond = TOP();
            int err;
            if (Py_IsFalse(cond)) {
                STACK_SHRINK(1);
                _Py_DECREF_NO_DEALLOC(cond);
                DISPATCH();
            }
            if (Py_IsTrue(cond)) {
                JUMPBY(oparg);
                DISPATCH();
            }
            err = PyObject_IsTrue(cond);
            if (err > 0) {
                JUMPBY(oparg);
            }
            else if (err == 0) {
                STACK_SHRINK(1);
                Py_DECREF(cond);
            }
            else
                goto error;
        }

        // stack effect: ( -- )
        inst(JUMP_BACKWARD_NO_INTERRUPT) {
            /* This bytecode is used in the `yield from` or `await` loop.
             * If there is an interrupt, we want it handled in the innermost
             * generator or coroutine, so we deliberately do not check it here.
             * (see bpo-30039).
             */
            JUMPBY(-oparg);
        }

        // stack effect: ( -- __0)
        inst(GET_LEN) {
            // PUSH(len(TOS))
            Py_ssize_t len_i = PyObject_Length(TOP());
            if (len_i < 0) {
                goto error;
            }
            PyObject *len_o = PyLong_FromSsize_t(len_i);
            if (len_o == NULL) {
                goto error;
            }
            PUSH(len_o);
        }

        // stack effect: (__0, __1 -- )
        inst(MATCH_CLASS) {
            // Pop TOS and TOS1. Set TOS to a tuple of attributes on success, or
            // None on failure.
            PyObject *names = POP();
            PyObject *type = POP();
            PyObject *subject = TOP();
            assert(PyTuple_CheckExact(names));
            PyObject *attrs = match_class(tstate, subject, type, oparg, names);
            Py_DECREF(names);
            Py_DECREF(type);
            if (attrs) {
                // Success!
                assert(PyTuple_CheckExact(attrs));
                SET_TOP(attrs);
            }
            else if (_PyErr_Occurred(tstate)) {
                // Error!
                goto error;
            }
            else {
                // Failure!
                Py_INCREF(Py_None);
                SET_TOP(Py_None);
            }
            Py_DECREF(subject);
        }

        // stack effect: ( -- __0)
        inst(MATCH_MAPPING) {
            PyObject *subject = TOP();
            int match = Py_TYPE(subject)->tp_flags & Py_TPFLAGS_MAPPING;
            PyObject *res = match ? Py_True : Py_False;
            Py_INCREF(res);
            PUSH(res);
            PREDICT(POP_JUMP_IF_FALSE);
        }

        // stack effect: ( -- __0)
        inst(MATCH_SEQUENCE) {
            PyObject *subject = TOP();
            int match = Py_TYPE(subject)->tp_flags & Py_TPFLAGS_SEQUENCE;
            PyObject *res = match ? Py_True : Py_False;
            Py_INCREF(res);
            PUSH(res);
            PREDICT(POP_JUMP_IF_FALSE);
        }

        // stack effect: ( -- __0)
        inst(MATCH_KEYS) {
            // On successful match, PUSH(values). Otherwise, PUSH(None).
            PyObject *keys = TOP();
            PyObject *subject = SECOND();
            PyObject *values_or_none = match_keys(tstate, subject, keys);
            if (values_or_none == NULL) {
                goto error;
            }
            PUSH(values_or_none);
        }

        // stack effect: ( -- )
        inst(GET_ITER) {
            /* before: [obj]; after [getiter(obj)] */
            PyObject *iterable = TOP();
            PyObject *iter = PyObject_GetIter(iterable);
            Py_DECREF(iterable);
            SET_TOP(iter);
            if (iter == NULL)
                goto error;
        }

        // stack effect: ( -- )
        inst(GET_YIELD_FROM_ITER) {
            /* before: [obj]; after [getiter(obj)] */
            PyObject *iterable = TOP();
            PyObject *iter;
            if (PyCoro_CheckExact(iterable)) {
                /* `iterable` is a coroutine */
                if (!(frame->f_code->co_flags & (CO_COROUTINE | CO_ITERABLE_COROUTINE))) {
                    /* and it is used in a 'yield from' expression of a
                       regular generator. */
                    Py_DECREF(iterable);
                    SET_TOP(NULL);
                    _PyErr_SetString(tstate, PyExc_TypeError,
                                     "cannot 'yield from' a coroutine object "
                                     "in a non-coroutine generator");
                    goto error;
                }
            }
            else if (!PyGen_CheckExact(iterable)) {
                /* `iterable` is not a generator. */
                iter = PyObject_GetIter(iterable);
                Py_DECREF(iterable);
                SET_TOP(iter);
                if (iter == NULL)
                    goto error;
            }
            PREDICT(LOAD_CONST);
        }

        // stack effect: ( -- __0)
        inst(FOR_ITER) {
            /* before: [iter]; after: [iter, iter()] *or* [] */
            PyObject *iter = TOP();
            PyObject *next = (*Py_TYPE(iter)->tp_iternext)(iter);
            if (next != NULL) {
                PUSH(next);
                JUMPBY(INLINE_CACHE_ENTRIES_FOR_ITER);
                DISPATCH();
            }
            if (_PyErr_Occurred(tstate)) {
                if (!_PyErr_ExceptionMatches(tstate, PyExc_StopIteration)) {
                    goto error;
                }
                else if (tstate->c_tracefunc != NULL) {
                    call_exc_trace(tstate->c_tracefunc, tstate->c_traceobj, tstate, frame);
                }
                _PyErr_Clear(tstate);
            }
            /* iterator ended normally */
            assert(_Py_OPCODE(next_instr[INLINE_CACHE_ENTRIES_FOR_ITER + oparg]) == END_FOR);
            STACK_SHRINK(1);
            Py_DECREF(iter);
            /* Skip END_FOR */
            JUMPBY(INLINE_CACHE_ENTRIES_FOR_ITER + oparg + 1);
        }

        // stack effect: ( -- __0)
        inst(FOR_ITER_ADAPTIVE) {
            assert(cframe.use_tracing == 0);
            _PyForIterCache *cache = (_PyForIterCache *)next_instr;
            if (ADAPTIVE_COUNTER_IS_ZERO(cache)) {
                next_instr--;
                _Py_Specialize_ForIter(TOP(), next_instr, oparg);
                DISPATCH_SAME_OPARG();
            }
            else {
                STAT_INC(FOR_ITER, deferred);
                DECREMENT_ADAPTIVE_COUNTER(cache);
                GO_TO_INSTRUCTION(FOR_ITER);
            }
        }

        // stack effect: ( -- __0)
        inst(FOR_ITER_LIST) {
            assert(cframe.use_tracing == 0);
            _PyListIterObject *it = (_PyListIterObject *)TOP();
            DEOPT_IF(Py_TYPE(it) != &PyListIter_Type, FOR_ITER);
            STAT_INC(FOR_ITER, hit);
            PyListObject *seq = it->it_seq;
            if (seq) {
                if (it->it_index < PyList_GET_SIZE(seq)) {
                    PyObject *next = PyList_GET_ITEM(seq, it->it_index++);
                    Py_INCREF(next);
                    PUSH(next);
                    JUMPBY(INLINE_CACHE_ENTRIES_FOR_ITER);
                    DISPATCH();
                }
                it->it_seq = NULL;
                Py_DECREF(seq);
            }
            STACK_SHRINK(1);
            Py_DECREF(it);
            JUMPBY(INLINE_CACHE_ENTRIES_FOR_ITER + oparg + 1);
        }

        // stack effect: ( -- __0)
        inst(FOR_ITER_RANGE) {
            assert(cframe.use_tracing == 0);
            _PyRangeIterObject *r = (_PyRangeIterObject *)TOP();
            DEOPT_IF(Py_TYPE(r) != &PyRangeIter_Type, FOR_ITER);
            STAT_INC(FOR_ITER, hit);
            _Py_CODEUNIT next = next_instr[INLINE_CACHE_ENTRIES_FOR_ITER];
            assert(_PyOpcode_Deopt[_Py_OPCODE(next)] == STORE_FAST);
            if (r->index >= r->len) {
                STACK_SHRINK(1);
                Py_DECREF(r);
                JUMPBY(INLINE_CACHE_ENTRIES_FOR_ITER + oparg + 1);
                DISPATCH();
            }
            long value = (long)(r->start +
                                (unsigned long)(r->index++) * r->step);
            if (_PyLong_AssignValue(&GETLOCAL(_Py_OPARG(next)), value) < 0) {
                goto error;
            }
            // The STORE_FAST is already done.
            JUMPBY(INLINE_CACHE_ENTRIES_FOR_ITER + 1);
        }

        inst(FOR_ITER_GEN) {
            assert(cframe.use_tracing == 0);
            PyGenObject *gen = (PyGenObject *)TOP();
            DEOPT_IF(Py_TYPE(gen) != &PyGen_Type, FOR_ITER);
            DEOPT_IF(gen->gi_frame_state >= FRAME_EXECUTING, FOR_ITER);
            STAT_INC(FOR_ITER, hit);
            _PyInterpreterFrame *gen_frame = (_PyInterpreterFrame *)gen->gi_iframe;
            _PyFrame_SetStackPointer(frame, stack_pointer);
            frame->yield_offset = oparg;
            JUMPBY(INLINE_CACHE_ENTRIES_FOR_ITER + oparg);
            assert(_Py_OPCODE(*next_instr) == END_FOR);
            frame->prev_instr = next_instr - 1;
            Py_INCREF(Py_None);
            _PyFrame_StackPush(gen_frame, Py_None);
            gen->gi_frame_state = FRAME_EXECUTING;
            gen->gi_exc_state.previous_item = tstate->exc_info;
            tstate->exc_info = &gen->gi_exc_state;
            gen_frame->previous = frame;
            gen_frame->is_entry = false;
            frame = cframe.current_frame = gen_frame;
            goto start_frame;
        }


        // stack effect: ( -- __0)
        inst(BEFORE_ASYNC_WITH) {
            PyObject *mgr = TOP();
            PyObject *res;
            PyObject *enter = _PyObject_LookupSpecial(mgr, &_Py_ID(__aenter__));
            if (enter == NULL) {
                if (!_PyErr_Occurred(tstate)) {
                    _PyErr_Format(tstate, PyExc_TypeError,
                                  "'%.200s' object does not support the "
                                  "asynchronous context manager protocol",
                                  Py_TYPE(mgr)->tp_name);
                }
                goto error;
            }
            PyObject *exit = _PyObject_LookupSpecial(mgr, &_Py_ID(__aexit__));
            if (exit == NULL) {
                if (!_PyErr_Occurred(tstate)) {
                    _PyErr_Format(tstate, PyExc_TypeError,
                                  "'%.200s' object does not support the "
                                  "asynchronous context manager protocol "
                                  "(missed __aexit__ method)",
                                  Py_TYPE(mgr)->tp_name);
                }
                Py_DECREF(enter);
                goto error;
            }
            SET_TOP(exit);
            Py_DECREF(mgr);
            res = _PyObject_CallNoArgs(enter);
            Py_DECREF(enter);
            if (res == NULL)
                goto error;
            PUSH(res);
            PREDICT(GET_AWAITABLE);
        }

        // stack effect: ( -- __0)
        inst(BEFORE_WITH) {
            PyObject *mgr = TOP();
            PyObject *res;
            PyObject *enter = _PyObject_LookupSpecial(mgr, &_Py_ID(__enter__));
            if (enter == NULL) {
                if (!_PyErr_Occurred(tstate)) {
                    _PyErr_Format(tstate, PyExc_TypeError,
                                  "'%.200s' object does not support the "
                                  "context manager protocol",
                                  Py_TYPE(mgr)->tp_name);
                }
                goto error;
            }
            PyObject *exit = _PyObject_LookupSpecial(mgr, &_Py_ID(__exit__));
            if (exit == NULL) {
                if (!_PyErr_Occurred(tstate)) {
                    _PyErr_Format(tstate, PyExc_TypeError,
                                  "'%.200s' object does not support the "
                                  "context manager protocol "
                                  "(missed __exit__ method)",
                                  Py_TYPE(mgr)->tp_name);
                }
                Py_DECREF(enter);
                goto error;
            }
            SET_TOP(exit);
            Py_DECREF(mgr);
            res = _PyObject_CallNoArgs(enter);
            Py_DECREF(enter);
            if (res == NULL) {
                goto error;
            }
            PUSH(res);
        }

        // stack effect: ( -- __0)
        inst(WITH_EXCEPT_START) {
            /* At the top of the stack are 4 values:
               - TOP = exc_info()
               - SECOND = previous exception
               - THIRD: lasti of exception in exc_info()
               - FOURTH: the context.__exit__ bound method
               We call FOURTH(type(TOP), TOP, GetTraceback(TOP)).
               Then we push the __exit__ return value.
            */
            PyObject *exit_func;
            PyObject *exc, *val, *tb, *res;

            val = TOP();
            assert(val && PyExceptionInstance_Check(val));
            exc = PyExceptionInstance_Class(val);
            tb = PyException_GetTraceback(val);
            Py_XDECREF(tb);
            assert(PyLong_Check(PEEK(3)));
            exit_func = PEEK(4);
            PyObject *stack[4] = {NULL, exc, val, tb};
            res = PyObject_Vectorcall(exit_func, stack + 1,
                    3 | PY_VECTORCALL_ARGUMENTS_OFFSET, NULL);
            if (res == NULL)
                goto error;

            PUSH(res);
        }

        // stack effect: ( -- __0)
        inst(PUSH_EXC_INFO) {
            PyObject *value = TOP();

            _PyErr_StackItem *exc_info = tstate->exc_info;
            if (exc_info->exc_value != NULL) {
                SET_TOP(exc_info->exc_value);
            }
            else {
                Py_INCREF(Py_None);
                SET_TOP(Py_None);
            }

            Py_INCREF(value);
            PUSH(value);
            assert(PyExceptionInstance_Check(value));
            exc_info->exc_value = value;

        }

        // error: LOAD_ATTR has irregular stack effect
        inst(LOAD_ATTR_METHOD_WITH_VALUES) {
            /* Cached method object */
            assert(cframe.use_tracing == 0);
            PyObject *self = TOP();
            PyTypeObject *self_cls = Py_TYPE(self);
            _PyLoadMethodCache *cache = (_PyLoadMethodCache *)next_instr;
            uint32_t type_version = read_u32(cache->type_version);
            assert(type_version != 0);
            DEOPT_IF(self_cls->tp_version_tag != type_version, LOAD_ATTR);
            assert(self_cls->tp_flags & Py_TPFLAGS_MANAGED_DICT);
            PyDictOrValues dorv = *_PyObject_DictOrValuesPointer(self);
            DEOPT_IF(!_PyDictOrValues_IsValues(dorv), LOAD_ATTR);
            PyHeapTypeObject *self_heap_type = (PyHeapTypeObject *)self_cls;
            DEOPT_IF(self_heap_type->ht_cached_keys->dk_version !=
                     read_u32(cache->keys_version), LOAD_ATTR);
            STAT_INC(LOAD_ATTR, hit);
            PyObject *res = read_obj(cache->descr);
            assert(res != NULL);
            assert(_PyType_HasFeature(Py_TYPE(res), Py_TPFLAGS_METHOD_DESCRIPTOR));
            Py_INCREF(res);
            SET_TOP(res);
            PUSH(self);
            JUMPBY(INLINE_CACHE_ENTRIES_LOAD_ATTR);
        }

        // error: LOAD_ATTR has irregular stack effect
        inst(LOAD_ATTR_METHOD_WITH_DICT) {
            /* Can be either a managed dict, or a tp_dictoffset offset.*/
            assert(cframe.use_tracing == 0);
            PyObject *self = TOP();
            PyTypeObject *self_cls = Py_TYPE(self);
            _PyLoadMethodCache *cache = (_PyLoadMethodCache *)next_instr;

            DEOPT_IF(self_cls->tp_version_tag != read_u32(cache->type_version),
                     LOAD_ATTR);
            /* Treat index as a signed 16 bit value */
            Py_ssize_t dictoffset = self_cls->tp_dictoffset;
            assert(dictoffset > 0);
            PyDictObject **dictptr = (PyDictObject**)(((char *)self)+dictoffset);
            PyDictObject *dict = *dictptr;
            DEOPT_IF(dict == NULL, LOAD_ATTR);
            DEOPT_IF(dict->ma_keys->dk_version != read_u32(cache->keys_version),
                     LOAD_ATTR);
            STAT_INC(LOAD_ATTR, hit);
            PyObject *res = read_obj(cache->descr);
            assert(res != NULL);
            assert(_PyType_HasFeature(Py_TYPE(res), Py_TPFLAGS_METHOD_DESCRIPTOR));
            Py_INCREF(res);
            SET_TOP(res);
            PUSH(self);
            JUMPBY(INLINE_CACHE_ENTRIES_LOAD_ATTR);
        }

        // error: LOAD_ATTR has irregular stack effect
        inst(LOAD_ATTR_METHOD_NO_DICT) {
            assert(cframe.use_tracing == 0);
            PyObject *self = TOP();
            PyTypeObject *self_cls = Py_TYPE(self);
            _PyLoadMethodCache *cache = (_PyLoadMethodCache *)next_instr;
            uint32_t type_version = read_u32(cache->type_version);
            DEOPT_IF(self_cls->tp_version_tag != type_version, LOAD_ATTR);
            assert(self_cls->tp_dictoffset == 0);
            STAT_INC(LOAD_ATTR, hit);
            PyObject *res = read_obj(cache->descr);
            assert(res != NULL);
            assert(_PyType_HasFeature(Py_TYPE(res), Py_TPFLAGS_METHOD_DESCRIPTOR));
            Py_INCREF(res);
            SET_TOP(res);
            PUSH(self);
            JUMPBY(INLINE_CACHE_ENTRIES_LOAD_ATTR);
        }

        // error: LOAD_ATTR has irregular stack effect
        inst(LOAD_ATTR_METHOD_LAZY_DICT) {
            assert(cframe.use_tracing == 0);
            PyObject *self = TOP();
            PyTypeObject *self_cls = Py_TYPE(self);
            _PyLoadMethodCache *cache = (_PyLoadMethodCache *)next_instr;
            uint32_t type_version = read_u32(cache->type_version);
            DEOPT_IF(self_cls->tp_version_tag != type_version, LOAD_ATTR);
            Py_ssize_t dictoffset = self_cls->tp_dictoffset;
            assert(dictoffset > 0);
            PyObject *dict = *(PyObject **)((char *)self + dictoffset);
            /* This object has a __dict__, just not yet created */
            DEOPT_IF(dict != NULL, LOAD_ATTR);
            STAT_INC(LOAD_ATTR, hit);
            PyObject *res = read_obj(cache->descr);
            assert(res != NULL);
            assert(_PyType_HasFeature(Py_TYPE(res), Py_TPFLAGS_METHOD_DESCRIPTOR));
            Py_INCREF(res);
            SET_TOP(res);
            PUSH(self);
            JUMPBY(INLINE_CACHE_ENTRIES_LOAD_ATTR);
        }

        // stack effect: (__0, __array[oparg] -- )
        inst(CALL_BOUND_METHOD_EXACT_ARGS) {
            DEOPT_IF(is_method(stack_pointer, oparg), CALL);
            PyObject *function = PEEK(oparg + 1);
            DEOPT_IF(Py_TYPE(function) != &PyMethod_Type, CALL);
            STAT_INC(CALL, hit);
            PyObject *meth = ((PyMethodObject *)function)->im_func;
            PyObject *self = ((PyMethodObject *)function)->im_self;
            Py_INCREF(meth);
            Py_INCREF(self);
            PEEK(oparg + 1) = self;
            PEEK(oparg + 2) = meth;
            Py_DECREF(function);
            GO_TO_INSTRUCTION(CALL_PY_EXACT_ARGS);
        }

        // stack effect: ( -- )
        inst(KW_NAMES) {
            assert(call_shape.kwnames == NULL);
            assert(oparg < PyTuple_GET_SIZE(consts));
            call_shape.kwnames = GETITEM(consts, oparg);
        }

        // stack effect: (__0, __array[oparg] -- )
        inst(CALL) {
            int total_args, is_meth;
            is_meth = is_method(stack_pointer, oparg);
            PyObject *function = PEEK(oparg + 1);
            if (!is_meth && Py_TYPE(function) == &PyMethod_Type) {
                PyObject *meth = ((PyMethodObject *)function)->im_func;
                PyObject *self = ((PyMethodObject *)function)->im_self;
                Py_INCREF(meth);
                Py_INCREF(self);
                PEEK(oparg+1) = self;
                PEEK(oparg+2) = meth;
                Py_DECREF(function);
                is_meth = 1;
            }
            total_args = oparg + is_meth;
            function = PEEK(total_args + 1);
            int positional_args = total_args - KWNAMES_LEN();
            // Check if the call can be inlined or not
            if (Py_TYPE(function) == &PyFunction_Type &&
                tstate->interp->eval_frame == NULL &&
                ((PyFunctionObject *)function)->vectorcall == _PyFunction_Vectorcall)
            {
                int code_flags = ((PyCodeObject*)PyFunction_GET_CODE(function))->co_flags;
                PyObject *locals = code_flags & CO_OPTIMIZED ? NULL : Py_NewRef(PyFunction_GET_GLOBALS(function));
                STACK_SHRINK(total_args);
                _PyInterpreterFrame *new_frame = _PyEvalFramePushAndInit(
                    tstate, (PyFunctionObject *)function, locals,
                    stack_pointer, positional_args, call_shape.kwnames
                );
                call_shape.kwnames = NULL;
                STACK_SHRINK(2-is_meth);
                // The frame has stolen all the arguments from the stack,
                // so there is no need to clean them up.
                if (new_frame == NULL) {
                    goto error;
                }
                _PyFrame_SetStackPointer(frame, stack_pointer);
                JUMPBY(INLINE_CACHE_ENTRIES_CALL);
                frame->prev_instr = next_instr - 1;
                new_frame->previous = frame;
                cframe.current_frame = frame = new_frame;
                CALL_STAT_INC(inlined_py_calls);
                goto start_frame;
            }
            /* Callable is not a normal Python function */
            PyObject *res;
            if (cframe.use_tracing) {
                res = trace_call_function(
                    tstate, function, stack_pointer-total_args,
                    positional_args, call_shape.kwnames);
            }
            else {
                res = PyObject_Vectorcall(
                    function, stack_pointer-total_args,
                    positional_args | PY_VECTORCALL_ARGUMENTS_OFFSET,
                    call_shape.kwnames);
            }
            call_shape.kwnames = NULL;
            assert((res != NULL) ^ (_PyErr_Occurred(tstate) != NULL));
            Py_DECREF(function);
            /* Clear the stack */
            STACK_SHRINK(total_args);
            for (int i = 0; i < total_args; i++) {
                Py_DECREF(stack_pointer[i]);
            }
            STACK_SHRINK(2-is_meth);
            PUSH(res);
            if (res == NULL) {
                goto error;
            }
            JUMPBY(INLINE_CACHE_ENTRIES_CALL);
            CHECK_EVAL_BREAKER();
        }

        // stack effect: (__0, __array[oparg] -- )
        inst(CALL_ADAPTIVE) {
            _PyCallCache *cache = (_PyCallCache *)next_instr;
            if (ADAPTIVE_COUNTER_IS_ZERO(cache)) {
                next_instr--;
                int is_meth = is_method(stack_pointer, oparg);
                int nargs = oparg + is_meth;
                PyObject *callable = PEEK(nargs + 1);
                int err = _Py_Specialize_Call(callable, next_instr, nargs,
                                              call_shape.kwnames);
                if (err < 0) {
                    goto error;
                }
                DISPATCH_SAME_OPARG();
            }
            else {
                STAT_INC(CALL, deferred);
                DECREMENT_ADAPTIVE_COUNTER(cache);
                GO_TO_INSTRUCTION(CALL);
            }
        }

        // stack effect: (__0, __array[oparg] -- )
        inst(CALL_PY_EXACT_ARGS) {
            assert(call_shape.kwnames == NULL);
            DEOPT_IF(tstate->interp->eval_frame, CALL);
            _PyCallCache *cache = (_PyCallCache *)next_instr;
            int is_meth = is_method(stack_pointer, oparg);
            int argcount = oparg + is_meth;
            PyObject *callable = PEEK(argcount + 1);
            DEOPT_IF(!PyFunction_Check(callable), CALL);
            PyFunctionObject *func = (PyFunctionObject *)callable;
            DEOPT_IF(func->func_version != read_u32(cache->func_version), CALL);
            PyCodeObject *code = (PyCodeObject *)func->func_code;
            DEOPT_IF(code->co_argcount != argcount, CALL);
            DEOPT_IF(!_PyThreadState_HasStackSpace(tstate, code->co_framesize), CALL);
            STAT_INC(CALL, hit);
            _PyInterpreterFrame *new_frame = _PyFrame_PushUnchecked(tstate, func);
            CALL_STAT_INC(inlined_py_calls);
            STACK_SHRINK(argcount);
            for (int i = 0; i < argcount; i++) {
                new_frame->localsplus[i] = stack_pointer[i];
            }
            for (int i = argcount; i < code->co_nlocalsplus; i++) {
                new_frame->localsplus[i] = NULL;
            }
            STACK_SHRINK(2-is_meth);
            _PyFrame_SetStackPointer(frame, stack_pointer);
            JUMPBY(INLINE_CACHE_ENTRIES_CALL);
            frame->prev_instr = next_instr - 1;
            new_frame->previous = frame;
            frame = cframe.current_frame = new_frame;
            goto start_frame;
        }

        // stack effect: (__0, __array[oparg] -- )
        inst(CALL_PY_WITH_DEFAULTS) {
            assert(call_shape.kwnames == NULL);
            DEOPT_IF(tstate->interp->eval_frame, CALL);
            _PyCallCache *cache = (_PyCallCache *)next_instr;
            int is_meth = is_method(stack_pointer, oparg);
            int argcount = oparg + is_meth;
            PyObject *callable = PEEK(argcount + 1);
            DEOPT_IF(!PyFunction_Check(callable), CALL);
            PyFunctionObject *func = (PyFunctionObject *)callable;
            DEOPT_IF(func->func_version != read_u32(cache->func_version), CALL);
            PyCodeObject *code = (PyCodeObject *)func->func_code;
            DEOPT_IF(argcount > code->co_argcount, CALL);
            int minargs = cache->min_args;
            DEOPT_IF(argcount < minargs, CALL);
            DEOPT_IF(!_PyThreadState_HasStackSpace(tstate, code->co_framesize), CALL);
            STAT_INC(CALL, hit);
            _PyInterpreterFrame *new_frame = _PyFrame_PushUnchecked(tstate, func);
            CALL_STAT_INC(inlined_py_calls);
            STACK_SHRINK(argcount);
            for (int i = 0; i < argcount; i++) {
                new_frame->localsplus[i] = stack_pointer[i];
            }
            for (int i = argcount; i < code->co_argcount; i++) {
                PyObject *def = PyTuple_GET_ITEM(func->func_defaults,
                                                 i - minargs);
                Py_INCREF(def);
                new_frame->localsplus[i] = def;
            }
            for (int i = code->co_argcount; i < code->co_nlocalsplus; i++) {
                new_frame->localsplus[i] = NULL;
            }
            STACK_SHRINK(2-is_meth);
            _PyFrame_SetStackPointer(frame, stack_pointer);
            JUMPBY(INLINE_CACHE_ENTRIES_CALL);
            frame->prev_instr = next_instr - 1;
            new_frame->previous = frame;
            frame = cframe.current_frame = new_frame;
            goto start_frame;
        }

        // stack effect: (__0, __array[oparg] -- )
        inst(CALL_NO_KW_TYPE_1) {
            assert(call_shape.kwnames == NULL);
            assert(cframe.use_tracing == 0);
            assert(oparg == 1);
            DEOPT_IF(is_method(stack_pointer, 1), CALL);
            PyObject *obj = TOP();
            PyObject *callable = SECOND();
            DEOPT_IF(callable != (PyObject *)&PyType_Type, CALL);
            STAT_INC(CALL, hit);
            JUMPBY(INLINE_CACHE_ENTRIES_CALL);
            PyObject *res = Py_NewRef(Py_TYPE(obj));
            Py_DECREF(callable);
            Py_DECREF(obj);
            STACK_SHRINK(2);
            SET_TOP(res);
        }

        // stack effect: (__0, __array[oparg] -- )
        inst(CALL_NO_KW_STR_1) {
            assert(call_shape.kwnames == NULL);
            assert(cframe.use_tracing == 0);
            assert(oparg == 1);
            DEOPT_IF(is_method(stack_pointer, 1), CALL);
            PyObject *callable = PEEK(2);
            DEOPT_IF(callable != (PyObject *)&PyUnicode_Type, CALL);
            STAT_INC(CALL, hit);
            JUMPBY(INLINE_CACHE_ENTRIES_CALL);
            PyObject *arg = TOP();
            PyObject *res = PyObject_Str(arg);
            Py_DECREF(arg);
            Py_DECREF(&PyUnicode_Type);
            STACK_SHRINK(2);
            SET_TOP(res);
            if (res == NULL) {
                goto error;
            }
            CHECK_EVAL_BREAKER();
        }

        // stack effect: (__0, __array[oparg] -- )
        inst(CALL_NO_KW_TUPLE_1) {
            assert(call_shape.kwnames == NULL);
            assert(oparg == 1);
            DEOPT_IF(is_method(stack_pointer, 1), CALL);
            PyObject *callable = PEEK(2);
            DEOPT_IF(callable != (PyObject *)&PyTuple_Type, CALL);
            STAT_INC(CALL, hit);
            JUMPBY(INLINE_CACHE_ENTRIES_CALL);
            PyObject *arg = TOP();
            PyObject *res = PySequence_Tuple(arg);
            Py_DECREF(arg);
            Py_DECREF(&PyTuple_Type);
            STACK_SHRINK(2);
            SET_TOP(res);
            if (res == NULL) {
                goto error;
            }
            CHECK_EVAL_BREAKER();
        }

        // stack effect: (__0, __array[oparg] -- )
        inst(CALL_BUILTIN_CLASS) {
            int is_meth = is_method(stack_pointer, oparg);
            int total_args = oparg + is_meth;
            int kwnames_len = KWNAMES_LEN();
            PyObject *callable = PEEK(total_args + 1);
            DEOPT_IF(!PyType_Check(callable), CALL);
            PyTypeObject *tp = (PyTypeObject *)callable;
            DEOPT_IF(tp->tp_vectorcall == NULL, CALL);
            STAT_INC(CALL, hit);
            JUMPBY(INLINE_CACHE_ENTRIES_CALL);
            STACK_SHRINK(total_args);
            PyObject *res = tp->tp_vectorcall((PyObject *)tp, stack_pointer,
                                              total_args-kwnames_len, call_shape.kwnames);
            call_shape.kwnames = NULL;
            /* Free the arguments. */
            for (int i = 0; i < total_args; i++) {
                Py_DECREF(stack_pointer[i]);
            }
            Py_DECREF(tp);
            STACK_SHRINK(1-is_meth);
            SET_TOP(res);
            if (res == NULL) {
                goto error;
            }
            CHECK_EVAL_BREAKER();
        }

        // stack effect: (__0, __array[oparg] -- )
        inst(CALL_NO_KW_BUILTIN_O) {
            assert(cframe.use_tracing == 0);
            /* Builtin METH_O functions */
            assert(call_shape.kwnames == NULL);
            int is_meth = is_method(stack_pointer, oparg);
            int total_args = oparg + is_meth;
            DEOPT_IF(total_args != 1, CALL);
            PyObject *callable = PEEK(total_args + 1);
            DEOPT_IF(!PyCFunction_CheckExact(callable), CALL);
            DEOPT_IF(PyCFunction_GET_FLAGS(callable) != METH_O, CALL);
            STAT_INC(CALL, hit);
            JUMPBY(INLINE_CACHE_ENTRIES_CALL);
            PyCFunction cfunc = PyCFunction_GET_FUNCTION(callable);
            // This is slower but CPython promises to check all non-vectorcall
            // function calls.
            if (_Py_EnterRecursiveCallTstate(tstate, " while calling a Python object")) {
                goto error;
            }
            PyObject *arg = TOP();
            PyObject *res = cfunc(PyCFunction_GET_SELF(callable), arg);
            _Py_LeaveRecursiveCallTstate(tstate);
            assert((res != NULL) ^ (_PyErr_Occurred(tstate) != NULL));

            Py_DECREF(arg);
            Py_DECREF(callable);
            STACK_SHRINK(2-is_meth);
            SET_TOP(res);
            if (res == NULL) {
                goto error;
            }
            CHECK_EVAL_BREAKER();
        }

        // stack effect: (__0, __array[oparg] -- )
        inst(CALL_NO_KW_BUILTIN_FAST) {
            assert(cframe.use_tracing == 0);
            /* Builtin METH_FASTCALL functions, without keywords */
            assert(call_shape.kwnames == NULL);
            int is_meth = is_method(stack_pointer, oparg);
            int total_args = oparg + is_meth;
            PyObject *callable = PEEK(total_args + 1);
            DEOPT_IF(!PyCFunction_CheckExact(callable), CALL);
            DEOPT_IF(PyCFunction_GET_FLAGS(callable) != METH_FASTCALL,
                CALL);
            STAT_INC(CALL, hit);
            JUMPBY(INLINE_CACHE_ENTRIES_CALL);
            PyCFunction cfunc = PyCFunction_GET_FUNCTION(callable);
            STACK_SHRINK(total_args);
            /* res = func(self, args, nargs) */
            PyObject *res = ((_PyCFunctionFast)(void(*)(void))cfunc)(
                PyCFunction_GET_SELF(callable),
                stack_pointer,
                total_args);
            assert((res != NULL) ^ (_PyErr_Occurred(tstate) != NULL));

            /* Free the arguments. */
            for (int i = 0; i < total_args; i++) {
                Py_DECREF(stack_pointer[i]);
            }
            STACK_SHRINK(2-is_meth);
            PUSH(res);
            Py_DECREF(callable);
            if (res == NULL) {
                /* Not deopting because this doesn't mean our optimization was
                   wrong. `res` can be NULL for valid reasons. Eg. getattr(x,
                   'invalid'). In those cases an exception is set, so we must
                   handle it.
                */
                goto error;
            }
            CHECK_EVAL_BREAKER();
        }

        // stack effect: (__0, __array[oparg] -- )
        inst(CALL_BUILTIN_FAST_WITH_KEYWORDS) {
            assert(cframe.use_tracing == 0);
            /* Builtin METH_FASTCALL | METH_KEYWORDS functions */
            int is_meth = is_method(stack_pointer, oparg);
            int total_args = oparg + is_meth;
            PyObject *callable = PEEK(total_args + 1);
            DEOPT_IF(!PyCFunction_CheckExact(callable), CALL);
            DEOPT_IF(PyCFunction_GET_FLAGS(callable) !=
                (METH_FASTCALL | METH_KEYWORDS), CALL);
            STAT_INC(CALL, hit);
            JUMPBY(INLINE_CACHE_ENTRIES_CALL);
            STACK_SHRINK(total_args);
            /* res = func(self, args, nargs, kwnames) */
            _PyCFunctionFastWithKeywords cfunc =
                (_PyCFunctionFastWithKeywords)(void(*)(void))
                PyCFunction_GET_FUNCTION(callable);
            PyObject *res = cfunc(
                PyCFunction_GET_SELF(callable),
                stack_pointer,
                total_args - KWNAMES_LEN(),
                call_shape.kwnames
            );
            assert((res != NULL) ^ (_PyErr_Occurred(tstate) != NULL));
            call_shape.kwnames = NULL;

            /* Free the arguments. */
            for (int i = 0; i < total_args; i++) {
                Py_DECREF(stack_pointer[i]);
            }
            STACK_SHRINK(2-is_meth);
            PUSH(res);
            Py_DECREF(callable);
            if (res == NULL) {
                goto error;
            }
            CHECK_EVAL_BREAKER();
        }

        // stack effect: (__0, __array[oparg] -- )
        inst(CALL_NO_KW_LEN) {
            assert(cframe.use_tracing == 0);
            assert(call_shape.kwnames == NULL);
            /* len(o) */
            int is_meth = is_method(stack_pointer, oparg);
            int total_args = oparg + is_meth;
            DEOPT_IF(total_args != 1, CALL);
            PyObject *callable = PEEK(total_args + 1);
            PyInterpreterState *interp = _PyInterpreterState_GET();
            DEOPT_IF(callable != interp->callable_cache.len, CALL);
            STAT_INC(CALL, hit);
            JUMPBY(INLINE_CACHE_ENTRIES_CALL);
            PyObject *arg = TOP();
            Py_ssize_t len_i = PyObject_Length(arg);
            if (len_i < 0) {
                goto error;
            }
            PyObject *res = PyLong_FromSsize_t(len_i);
            assert((res != NULL) ^ (_PyErr_Occurred(tstate) != NULL));

            STACK_SHRINK(2-is_meth);
            SET_TOP(res);
            Py_DECREF(callable);
            Py_DECREF(arg);
            if (res == NULL) {
                goto error;
            }
        }

        // stack effect: (__0, __array[oparg] -- )
        inst(CALL_NO_KW_ISINSTANCE) {
            assert(cframe.use_tracing == 0);
            assert(call_shape.kwnames == NULL);
            /* isinstance(o, o2) */
            int is_meth = is_method(stack_pointer, oparg);
            int total_args = oparg + is_meth;
            PyObject *callable = PEEK(total_args + 1);
            DEOPT_IF(total_args != 2, CALL);
            PyInterpreterState *interp = _PyInterpreterState_GET();
            DEOPT_IF(callable != interp->callable_cache.isinstance, CALL);
            STAT_INC(CALL, hit);
            JUMPBY(INLINE_CACHE_ENTRIES_CALL);
            PyObject *cls = POP();
            PyObject *inst = TOP();
            int retval = PyObject_IsInstance(inst, cls);
            if (retval < 0) {
                Py_DECREF(cls);
                goto error;
            }
            PyObject *res = PyBool_FromLong(retval);
            assert((res != NULL) ^ (_PyErr_Occurred(tstate) != NULL));

            STACK_SHRINK(2-is_meth);
            SET_TOP(res);
            Py_DECREF(inst);
            Py_DECREF(cls);
            Py_DECREF(callable);
            if (res == NULL) {
                goto error;
            }
        }

        // stack effect: (__0, __array[oparg] -- )
        inst(CALL_NO_KW_LIST_APPEND) {
            assert(cframe.use_tracing == 0);
            assert(call_shape.kwnames == NULL);
            assert(oparg == 1);
            PyObject *callable = PEEK(3);
            PyInterpreterState *interp = _PyInterpreterState_GET();
            DEOPT_IF(callable != interp->callable_cache.list_append, CALL);
            PyObject *list = SECOND();
            DEOPT_IF(!PyList_Check(list), CALL);
            STAT_INC(CALL, hit);
            // CALL + POP_TOP
            JUMPBY(INLINE_CACHE_ENTRIES_CALL + 1);
            assert(_Py_OPCODE(next_instr[-1]) == POP_TOP);
            PyObject *arg = POP();
            if (_PyList_AppendTakeRef((PyListObject *)list, arg) < 0) {
                goto error;
            }
            STACK_SHRINK(2);
            Py_DECREF(list);
            Py_DECREF(callable);
        }

        // stack effect: (__0, __array[oparg] -- )
        inst(CALL_NO_KW_METHOD_DESCRIPTOR_O) {
            assert(call_shape.kwnames == NULL);
            int is_meth = is_method(stack_pointer, oparg);
            int total_args = oparg + is_meth;
            PyMethodDescrObject *callable =
                (PyMethodDescrObject *)PEEK(total_args + 1);
            DEOPT_IF(total_args != 2, CALL);
            DEOPT_IF(!Py_IS_TYPE(callable, &PyMethodDescr_Type), CALL);
            PyMethodDef *meth = callable->d_method;
            DEOPT_IF(meth->ml_flags != METH_O, CALL);
            PyObject *arg = TOP();
            PyObject *self = SECOND();
            DEOPT_IF(!Py_IS_TYPE(self, callable->d_common.d_type), CALL);
            STAT_INC(CALL, hit);
            JUMPBY(INLINE_CACHE_ENTRIES_CALL);
            PyCFunction cfunc = meth->ml_meth;
            // This is slower but CPython promises to check all non-vectorcall
            // function calls.
            if (_Py_EnterRecursiveCallTstate(tstate, " while calling a Python object")) {
                goto error;
            }
            PyObject *res = cfunc(self, arg);
            _Py_LeaveRecursiveCallTstate(tstate);
            assert((res != NULL) ^ (_PyErr_Occurred(tstate) != NULL));
            Py_DECREF(self);
            Py_DECREF(arg);
            STACK_SHRINK(oparg + 1);
            SET_TOP(res);
            Py_DECREF(callable);
            if (res == NULL) {
                goto error;
            }
            CHECK_EVAL_BREAKER();
        }

        // stack effect: (__0, __array[oparg] -- )
        inst(CALL_METHOD_DESCRIPTOR_FAST_WITH_KEYWORDS) {
            int is_meth = is_method(stack_pointer, oparg);
            int total_args = oparg + is_meth;
            PyMethodDescrObject *callable =
                (PyMethodDescrObject *)PEEK(total_args + 1);
            DEOPT_IF(!Py_IS_TYPE(callable, &PyMethodDescr_Type), CALL);
            PyMethodDef *meth = callable->d_method;
            DEOPT_IF(meth->ml_flags != (METH_FASTCALL|METH_KEYWORDS), CALL);
            PyTypeObject *d_type = callable->d_common.d_type;
            PyObject *self = PEEK(total_args);
            DEOPT_IF(!Py_IS_TYPE(self, d_type), CALL);
            STAT_INC(CALL, hit);
            JUMPBY(INLINE_CACHE_ENTRIES_CALL);
            int nargs = total_args-1;
            STACK_SHRINK(nargs);
            _PyCFunctionFastWithKeywords cfunc =
                (_PyCFunctionFastWithKeywords)(void(*)(void))meth->ml_meth;
            PyObject *res = cfunc(self, stack_pointer, nargs - KWNAMES_LEN(),
                                  call_shape.kwnames);
            assert((res != NULL) ^ (_PyErr_Occurred(tstate) != NULL));
            call_shape.kwnames = NULL;

            /* Free the arguments. */
            for (int i = 0; i < nargs; i++) {
                Py_DECREF(stack_pointer[i]);
            }
            Py_DECREF(self);
            STACK_SHRINK(2-is_meth);
            SET_TOP(res);
            Py_DECREF(callable);
            if (res == NULL) {
                goto error;
            }
            CHECK_EVAL_BREAKER();
        }

        // stack effect: (__0, __array[oparg] -- )
        inst(CALL_NO_KW_METHOD_DESCRIPTOR_NOARGS) {
            assert(call_shape.kwnames == NULL);
            assert(oparg == 0 || oparg == 1);
            int is_meth = is_method(stack_pointer, oparg);
            int total_args = oparg + is_meth;
            DEOPT_IF(total_args != 1, CALL);
            PyMethodDescrObject *callable = (PyMethodDescrObject *)SECOND();
            DEOPT_IF(!Py_IS_TYPE(callable, &PyMethodDescr_Type), CALL);
            PyMethodDef *meth = callable->d_method;
            PyObject *self = TOP();
            DEOPT_IF(!Py_IS_TYPE(self, callable->d_common.d_type), CALL);
            DEOPT_IF(meth->ml_flags != METH_NOARGS, CALL);
            STAT_INC(CALL, hit);
            JUMPBY(INLINE_CACHE_ENTRIES_CALL);
            PyCFunction cfunc = meth->ml_meth;
            // This is slower but CPython promises to check all non-vectorcall
            // function calls.
            if (_Py_EnterRecursiveCallTstate(tstate, " while calling a Python object")) {
                goto error;
            }
            PyObject *res = cfunc(self, NULL);
            _Py_LeaveRecursiveCallTstate(tstate);
            assert((res != NULL) ^ (_PyErr_Occurred(tstate) != NULL));
            Py_DECREF(self);
            STACK_SHRINK(oparg + 1);
            SET_TOP(res);
            Py_DECREF(callable);
            if (res == NULL) {
                goto error;
            }
            CHECK_EVAL_BREAKER();
        }

        // stack effect: (__0, __array[oparg] -- )
        inst(CALL_NO_KW_METHOD_DESCRIPTOR_FAST) {
            assert(call_shape.kwnames == NULL);
            int is_meth = is_method(stack_pointer, oparg);
            int total_args = oparg + is_meth;
            PyMethodDescrObject *callable =
                (PyMethodDescrObject *)PEEK(total_args + 1);
            /* Builtin METH_FASTCALL methods, without keywords */
            DEOPT_IF(!Py_IS_TYPE(callable, &PyMethodDescr_Type), CALL);
            PyMethodDef *meth = callable->d_method;
            DEOPT_IF(meth->ml_flags != METH_FASTCALL, CALL);
            PyObject *self = PEEK(total_args);
            DEOPT_IF(!Py_IS_TYPE(self, callable->d_common.d_type), CALL);
            STAT_INC(CALL, hit);
            JUMPBY(INLINE_CACHE_ENTRIES_CALL);
            _PyCFunctionFast cfunc =
                (_PyCFunctionFast)(void(*)(void))meth->ml_meth;
            int nargs = total_args-1;
            STACK_SHRINK(nargs);
            PyObject *res = cfunc(self, stack_pointer, nargs);
            assert((res != NULL) ^ (_PyErr_Occurred(tstate) != NULL));
            /* Clear the stack of the arguments. */
            for (int i = 0; i < nargs; i++) {
                Py_DECREF(stack_pointer[i]);
            }
            Py_DECREF(self);
            STACK_SHRINK(2-is_meth);
            SET_TOP(res);
            Py_DECREF(callable);
            if (res == NULL) {
                goto error;
            }
            CHECK_EVAL_BREAKER();
        }

        // error: CALL_FUNCTION_EX has irregular stack effect
        inst(CALL_FUNCTION_EX) {
            PyObject *func, *callargs, *kwargs = NULL, *result;
            if (oparg & 0x01) {
                kwargs = POP();
                // DICT_MERGE is called before this opcode if there are kwargs.
                // It converts all dict subtypes in kwargs into regular dicts.
                assert(PyDict_CheckExact(kwargs));
            }
            callargs = POP();
            func = TOP();
            if (!PyTuple_CheckExact(callargs)) {
                if (check_args_iterable(tstate, func, callargs) < 0) {
                    Py_DECREF(callargs);
                    goto error;
                }
                Py_SETREF(callargs, PySequence_Tuple(callargs));
                if (callargs == NULL) {
                    goto error;
                }
            }
            assert(PyTuple_CheckExact(callargs));

            result = do_call_core(tstate, func, callargs, kwargs, cframe.use_tracing);
            Py_DECREF(func);
            Py_DECREF(callargs);
            Py_XDECREF(kwargs);

            STACK_SHRINK(1);
            assert(TOP() == NULL);
            SET_TOP(result);
            if (result == NULL) {
                goto error;
            }
            CHECK_EVAL_BREAKER();
        }

        // error: MAKE_FUNCTION has irregular stack effect
        inst(MAKE_FUNCTION) {
            PyObject *codeobj = POP();
            PyFunctionObject *func = (PyFunctionObject *)
                PyFunction_New(codeobj, GLOBALS());

            Py_DECREF(codeobj);
            if (func == NULL) {
                goto error;
            }

            if (oparg & 0x08) {
                assert(PyTuple_CheckExact(TOP()));
                func->func_closure = POP();
            }
            if (oparg & 0x04) {
                assert(PyTuple_CheckExact(TOP()));
                func->func_annotations = POP();
            }
            if (oparg & 0x02) {
                assert(PyDict_CheckExact(TOP()));
                func->func_kwdefaults = POP();
            }
            if (oparg & 0x01) {
                assert(PyTuple_CheckExact(TOP()));
                func->func_defaults = POP();
            }

            PUSH((PyObject *)func);
        }

        // stack effect: ( -- )
        inst(RETURN_GENERATOR) {
            assert(PyFunction_Check(frame->f_funcobj));
            PyFunctionObject *func = (PyFunctionObject *)frame->f_funcobj;
            PyGenObject *gen = (PyGenObject *)_Py_MakeCoro(func);
            if (gen == NULL) {
                goto error;
            }
            assert(EMPTY());
            _PyFrame_SetStackPointer(frame, stack_pointer);
            _PyInterpreterFrame *gen_frame = (_PyInterpreterFrame *)gen->gi_iframe;
            _PyFrame_Copy(frame, gen_frame);
            assert(frame->frame_obj == NULL);
            gen->gi_frame_state = FRAME_CREATED;
            gen_frame->owner = FRAME_OWNED_BY_GENERATOR;
            _Py_LeaveRecursiveCallPy(tstate);
            if (!frame->is_entry) {
                _PyInterpreterFrame *prev = frame->previous;
                _PyThreadState_PopFrame(tstate, frame);
                frame = cframe.current_frame = prev;
                _PyFrame_StackPush(frame, (PyObject *)gen);
                goto resume_frame;
            }
            _Py_LeaveRecursiveCallTstate(tstate);
            /* Make sure that frame is in a valid state */
            frame->stacktop = 0;
            frame->f_locals = NULL;
            Py_INCREF(frame->f_funcobj);
            Py_INCREF(frame->f_code);
            /* Restore previous cframe and return. */
            tstate->cframe = cframe.previous;
            tstate->cframe->use_tracing = cframe.use_tracing;
            assert(tstate->cframe->current_frame == frame->previous);
            assert(!_PyErr_Occurred(tstate));
            return (PyObject *)gen;
        }

        // error: BUILD_SLICE has irregular stack effect
        inst(BUILD_SLICE) {
            PyObject *start, *stop, *step, *slice;
            if (oparg == 3)
                step = POP();
            else
                step = NULL;
            stop = POP();
            start = TOP();
            slice = PySlice_New(start, stop, step);
            Py_DECREF(start);
            Py_DECREF(stop);
            Py_XDECREF(step);
            SET_TOP(slice);
            if (slice == NULL)
                goto error;
        }

        // error: FORMAT_VALUE has irregular stack effect
        inst(FORMAT_VALUE) {
            /* Handles f-string value formatting. */
            PyObject *result;
            PyObject *fmt_spec;
            PyObject *value;
            PyObject *(*conv_fn)(PyObject *);
            int which_conversion = oparg & FVC_MASK;
            int have_fmt_spec = (oparg & FVS_MASK) == FVS_HAVE_SPEC;

            fmt_spec = have_fmt_spec ? POP() : NULL;
            value = POP();

            /* See if any conversion is specified. */
            switch (which_conversion) {
            case FVC_NONE:  conv_fn = NULL;           break;
            case FVC_STR:   conv_fn = PyObject_Str;   break;
            case FVC_REPR:  conv_fn = PyObject_Repr;  break;
            case FVC_ASCII: conv_fn = PyObject_ASCII; break;
            default:
                _PyErr_Format(tstate, PyExc_SystemError,
                              "unexpected conversion flag %d",
                              which_conversion);
                goto error;
            }

            /* If there's a conversion function, call it and replace
               value with that result. Otherwise, just use value,
               without conversion. */
            if (conv_fn != NULL) {
                result = conv_fn(value);
                Py_DECREF(value);
                if (result == NULL) {
                    Py_XDECREF(fmt_spec);
                    goto error;
                }
                value = result;
            }

            /* If value is a unicode object, and there's no fmt_spec,
               then we know the result of format(value) is value
               itself. In that case, skip calling format(). I plan to
               move this optimization in to PyObject_Format()
               itself. */
            if (PyUnicode_CheckExact(value) && fmt_spec == NULL) {
                /* Do nothing, just transfer ownership to result. */
                result = value;
            } else {
                /* Actually call format(). */
                result = PyObject_Format(value, fmt_spec);
                Py_DECREF(value);
                Py_XDECREF(fmt_spec);
                if (result == NULL) {
                    goto error;
                }
            }

            PUSH(result);
        }

        // stack effect: ( -- __0)
        inst(COPY) {
            assert(oparg != 0);
            PyObject *peek = PEEK(oparg);
            Py_INCREF(peek);
            PUSH(peek);
        }

        // stack effect: (__0 -- )
        inst(BINARY_OP) {
            PyObject *rhs = POP();
            PyObject *lhs = TOP();
            assert(0 <= oparg);
            assert((unsigned)oparg < Py_ARRAY_LENGTH(binary_ops));
            assert(binary_ops[oparg]);
            PyObject *res = binary_ops[oparg](lhs, rhs);
            Py_DECREF(lhs);
            Py_DECREF(rhs);
            SET_TOP(res);
            if (res == NULL) {
                goto error;
            }
            JUMPBY(INLINE_CACHE_ENTRIES_BINARY_OP);
        }

        // stack effect: (__0 -- )
        inst(BINARY_OP_ADAPTIVE) {
            assert(cframe.use_tracing == 0);
            _PyBinaryOpCache *cache = (_PyBinaryOpCache *)next_instr;
            if (ADAPTIVE_COUNTER_IS_ZERO(cache)) {
                PyObject *lhs = SECOND();
                PyObject *rhs = TOP();
                next_instr--;
                _Py_Specialize_BinaryOp(lhs, rhs, next_instr, oparg, &GETLOCAL(0));
                DISPATCH_SAME_OPARG();
            }
            else {
                STAT_INC(BINARY_OP, deferred);
                DECREMENT_ADAPTIVE_COUNTER(cache);
                GO_TO_INSTRUCTION(BINARY_OP);
            }
        }

        // stack effect: ( -- )
        inst(SWAP) {
            assert(oparg != 0);
            PyObject *top = TOP();
            SET_TOP(PEEK(oparg));
            PEEK(oparg) = top;
        }

        // stack effect: ( -- )
        inst(EXTENDED_ARG) {
            assert(oparg);
            oparg <<= 8;
            oparg |= _Py_OPARG(*next_instr);
            // We might be tracing. To avoid breaking tracing guarantees in
            // quickened instructions, always deoptimize the next opcode:
            opcode = _PyOpcode_Deopt[_Py_OPCODE(*next_instr)];
            PRE_DISPATCH_GOTO();
            // CPython hasn't traced the following instruction historically
            // (DO_TRACING would clobber our extended oparg anyways), so just
            // skip our usual cframe.use_tracing check before dispatch. Also,
            // make sure the next instruction isn't a RESUME, since that needs
            // to trace properly (and shouldn't have an extended arg anyways):
            assert(opcode != RESUME);
            DISPATCH_GOTO();
        }

        // stack effect: ( -- )
        inst(EXTENDED_ARG_QUICK) {
            assert(cframe.use_tracing == 0);
            assert(oparg);
            int oldoparg = oparg;
            NEXTOPARG();
            oparg |= oldoparg << 8;
            DISPATCH_GOTO();
        }

        // stack effect: ( -- )
        inst(CACHE) {
            Py_UNREACHABLE();
        }


// END BYTECODES //

    }
 error:
 exception_unwind:
 handle_eval_breaker:
 resume_frame:
 resume_with_error:
 start_frame:
 unbound_local_error:
    ;
}

// Families go below this point //

family(binary_op) = {
    BINARY_OP, BINARY_OP_ADAPTIVE, BINARY_OP_ADD_FLOAT,
    BINARY_OP_ADD_INT, BINARY_OP_ADD_UNICODE, BINARY_OP_INPLACE_ADD_UNICODE,
    BINARY_OP_MULTIPLY_FLOAT, BINARY_OP_MULTIPLY_INT, BINARY_OP_SUBTRACT_FLOAT,
    BINARY_OP_SUBTRACT_INT };
family(binary_subscr) = {
    BINARY_SUBSCR, BINARY_SUBSCR_ADAPTIVE, BINARY_SUBSCR_DICT,
    BINARY_SUBSCR_GETITEM, BINARY_SUBSCR_LIST_INT, BINARY_SUBSCR_TUPLE_INT };
family(call) = {
    CALL, CALL_ADAPTIVE, CALL_PY_EXACT_ARGS,
    CALL_PY_WITH_DEFAULTS, CALL_BOUND_METHOD_EXACT_ARGS, CALL_BUILTIN_CLASS,
    CALL_BUILTIN_FAST_WITH_KEYWORDS, CALL_METHOD_DESCRIPTOR_FAST_WITH_KEYWORDS, CALL_NO_KW_BUILTIN_FAST,
    CALL_NO_KW_BUILTIN_O, CALL_NO_KW_ISINSTANCE, CALL_NO_KW_LEN,
    CALL_NO_KW_LIST_APPEND, CALL_NO_KW_METHOD_DESCRIPTOR_FAST, CALL_NO_KW_METHOD_DESCRIPTOR_NOARGS,
    CALL_NO_KW_METHOD_DESCRIPTOR_O, CALL_NO_KW_STR_1, CALL_NO_KW_TUPLE_1,
    CALL_NO_KW_TYPE_1 };
family(compare_op) = {
    COMPARE_OP, COMPARE_OP_ADAPTIVE, COMPARE_OP_FLOAT_JUMP,
    COMPARE_OP_INT_JUMP, COMPARE_OP_STR_JUMP };
family(extended_arg) = { EXTENDED_ARG, EXTENDED_ARG_QUICK };
family(for_iter) = {
    FOR_ITER, FOR_ITER_ADAPTIVE, FOR_ITER_LIST,
    FOR_ITER_RANGE };
family(load_attr) = {
    LOAD_ATTR, LOAD_ATTR_ADAPTIVE, LOAD_ATTR_CLASS,
    LOAD_ATTR_GETATTRIBUTE_OVERRIDDEN, LOAD_ATTR_INSTANCE_VALUE, LOAD_ATTR_MODULE,
    LOAD_ATTR_PROPERTY, LOAD_ATTR_SLOT, LOAD_ATTR_WITH_HINT,
    LOAD_ATTR_METHOD_LAZY_DICT, LOAD_ATTR_METHOD_NO_DICT, LOAD_ATTR_METHOD_WITH_DICT,
    LOAD_ATTR_METHOD_WITH_VALUES };
family(load_const) = { LOAD_CONST, LOAD_CONST__LOAD_FAST };
family(load_fast) = { LOAD_FAST, LOAD_FAST__LOAD_CONST, LOAD_FAST__LOAD_FAST };
family(load_global) = {
    LOAD_GLOBAL, LOAD_GLOBAL_ADAPTIVE, LOAD_GLOBAL_BUILTIN,
    LOAD_GLOBAL_MODULE };
family(store_attr) = {
    STORE_ATTR, STORE_ATTR_ADAPTIVE, STORE_ATTR_INSTANCE_VALUE,
    STORE_ATTR_SLOT, STORE_ATTR_WITH_HINT };
family(store_fast) = { STORE_FAST, STORE_FAST__LOAD_FAST, STORE_FAST__STORE_FAST };
family(store_subscr) = {
    STORE_SUBSCR, STORE_SUBSCR_ADAPTIVE, STORE_SUBSCR_DICT,
    STORE_SUBSCR_LIST_INT };
family(unpack_sequence) = {
    UNPACK_SEQUENCE, UNPACK_SEQUENCE_ADAPTIVE, UNPACK_SEQUENCE_LIST,
    UNPACK_SEQUENCE_TUPLE, UNPACK_SEQUENCE_TWO_TUPLE };