/*
 * Bootstrap KL 
 * Copyright (C) 2012 Dmitry Cherkassov
 * BSD license
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <stdint.h>
#include <math.h>
#include <unistd.h>


/**************************** MODEL ******************************/

typedef enum {THE_EMPTY_LIST, BOOLEAN, SYMBOL, ARGUMENT, FIXNUM, DOUBLE,
              CHARACTER, STRING, PAIR, PRIMITIVE_PROC,
              COMPOUND_PROC, FREEZE, JMP_ENV, EXCEPTION, VECTOR, FILE_STREAM} object_type;

typedef enum {FILE_IN, FILE_OUT} fstream_type;


typedef struct object {
	object_type type;
	union {
		struct {
			char value;
		} boolean;
		struct {
			char *value;
			char is_argument;
			char is_func;
		} symbol;
		struct {
			/* long value; */
			int64_t value;
		} fixnum;
		struct {
			/* long value; */
			double value;
		} double_num;
		struct {
			char value;
			/* int64_t value; */
		} character;
		struct {
			char *value;
		} string;
		struct {
			struct object *car;
			struct object *cdr;
		} pair;
		struct {
			struct object *(*fn)(struct object *arguments);
		} primitive_proc;
		struct {
			struct object *parameters;
			struct object *body;
			struct object *env;
		} compound_proc;
		struct {
			struct object *exp;
		} freeze;
		struct {
			jmp_buf jmp_env;
			struct object *exception;
		} context;
		struct {
			char *msg;
		} exception;
		struct {
			struct object **vec;
			unsigned int limit;
		} vector;
		struct {
			FILE *fd;
			char *path;
			fstream_type type;
		} file_stream;

	} data;
} object;

char *strdup(const char *s);

/* EF - eval flag */
#define EF_NULL 0
#define EF_ARGUMENTS 1
#define EF_TOPLEVEL 2

#define MAX_FPATH 256

void print(object *obj);
void println(object *obj);

/* no GC so truely "unlimited extent" */
object *alloc_object(void) {
	object *obj;

	obj = malloc(sizeof(object));
	if (obj == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(1);
	}
	return obj;
}

object *the_empty_list;
object *false;
object *true;
object *symbol_table;
object *quote_symbol;
object *defun_symbol;
object *set_symbol;
object *ok_symbol;
object *if_symbol;
object *lambda_symbol;
object *begin_symbol;
object *cond_symbol;
object *else_symbol;
object *let_symbol;
object *value_symbol;
object *freeze_symbol;
object *trap_error_symbol;
object *simple_error_symbol;
object *eval_without_macros_symbol;
object *fail_symbol;

object *in_symbol;
object *out_symbol; 
object *file_symbol;

object *run_symbol;

object *the_empty_environment;
object *funcs_env;
object *params_env;
object *symbols_env;
object *jmp_envs;

object *homedir_obj;

object *cons(object *car, object *cdr);
object *car(object *pair);
object *cdr(object *pair);




void dump_object(object *obj, char *str);

 char is_the_empty_list(object *obj) {
	return obj == the_empty_list;
}

char is_boolean(object *obj) {
	return obj->type == BOOLEAN;
}

char is_false(object *obj) {
	return obj == false;
}

char is_true(object *obj) {
	return !is_false(obj);
}

object *make_symbol(char *value) {
	object *obj;
	object *element;
    
	/* search for the symbol in the symbol table */
	element = symbol_table;
	while (!is_the_empty_list(element)) {
		if (strcmp(car(element)->data.symbol.value, value) == 0) {
			return car(element);
		}
		element = cdr(element);
	};
    
	/* create the symbol and add it to the symbol table */
	obj = alloc_object();
	obj->type = SYMBOL;
	obj->data.symbol.value = malloc(strlen(value) + 1);
	obj->data.symbol.is_argument = 0;
	obj->data.symbol.is_func = 0;
	if (obj->data.symbol.value == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(1);
	}
	strcpy(obj->data.symbol.value, value);
	symbol_table = cons(obj, symbol_table);
	return obj;
}

object *make_argument(char *value)
{
	object *obj = make_symbol(value);
	obj->data.symbol.is_argument = 1;
	return obj;
}

char is_symbol(object *obj) {
	return obj->type == SYMBOL;
}

char is_argument(object *obj) {
	return obj->data.symbol.is_argument == 1;
}


object *make_fixnum(long value) {
	object *obj;

	obj = alloc_object();
	obj->type = FIXNUM;
	obj->data.fixnum.value = value;
	return obj;
}

object *make_double(double value) {
	object *obj;

	obj = alloc_object();
	obj->type = DOUBLE;
	obj->data.double_num.value = value;
	return obj;
}


object *make_boolean(char value) {
	object *obj = (value) ? true : false;
	return obj;
}

char is_fixnum(object *obj) {
	return obj->type == FIXNUM;
}

char is_double(object *obj) {
	return obj->type == DOUBLE;
}

char is_number(object *obj) {
	return (obj->type == DOUBLE) || (obj->type == FIXNUM);
}

object *make_character(char value) {
	object *obj;

	obj = alloc_object();
	obj->type = CHARACTER;
	obj->data.character.value = value;
	return obj;
}

char is_character(object *obj) {
	return obj->type == CHARACTER;
}

object *make_string(char *value) {
	object *obj;

	obj = alloc_object();
	obj->type = STRING;
	obj->data.string.value = malloc(strlen(value) + 1);
	if (obj->data.string.value == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(1);
	}
	strcpy(obj->data.string.value, value);
	return obj;
}

object *make_file_stream(FILE *fd, const char *path, fstream_type fst) {
	object *obj;
	obj = alloc_object();
	obj->type = FILE_STREAM;
	obj->data.file_stream.fd = fd;
	obj->data.file_stream.path = strdup(path);
	obj->data.file_stream.type = fst;

	return obj;
}



char is_string(object *obj) {
	return obj->type == STRING;
}

char is_empty_list(object *obj) {
	return obj->type == THE_EMPTY_LIST;
}

object *cons(object *car, object *cdr) {
	object *obj;
    
	obj = alloc_object();
	obj->type = PAIR;
	obj->data.pair.car = car;
	obj->data.pair.cdr = cdr;
	return obj;
}

char is_pair(object *obj) {
	return obj->type == PAIR;
}

object *car(object *pair) {
	return pair->data.pair.car;
}

void set_car(object *obj, object* value) {
	obj->data.pair.car = value;
}

object *cdr(object *pair) {
	return pair->data.pair.cdr;
}

void set_cdr(object *obj, object* value) {
	obj->data.pair.cdr = value;
}

#define caar(obj)   car(car(obj))
#define cadr(obj)   car(cdr(obj))
#define cdar(obj)   cdr(car(obj))
#define cddr(obj)   cdr(cdr(obj))
#define caaar(obj)  car(car(car(obj)))
#define caadr(obj)  car(car(cdr(obj)))
#define cadar(obj)  car(cdr(car(obj)))
#define caddr(obj)  car(cdr(cdr(obj)))
#define cdaar(obj)  cdr(car(car(obj)))
#define cdadr(obj)  cdr(car(cdr(obj)))
#define cddar(obj)  cdr(cdr(car(obj)))
#define cdddr(obj)  cdr(cdr(cdr(obj)))
#define caaaar(obj) car(car(car(car(obj))))
#define caaadr(obj) car(car(car(cdr(obj))))
#define caadar(obj) car(car(cdr(car(obj))))
#define caaddr(obj) car(car(cdr(cdr(obj))))
#define cadaar(obj) car(cdr(car(car(obj))))
#define cadadr(obj) car(cdr(car(cdr(obj))))
#define caddar(obj) car(cdr(cdr(car(obj))))
#define cadddr(obj) car(cdr(cdr(cdr(obj))))
#define cdaaar(obj) cdr(car(car(car(obj))))
#define cdaadr(obj) cdr(car(car(cdr(obj))))
#define cdadar(obj) cdr(car(cdr(car(obj))))
#define cdaddr(obj) cdr(car(cdr(cdr(obj))))
#define cddaar(obj) cdr(cdr(car(car(obj))))
#define cddadr(obj) cdr(cdr(car(cdr(obj))))
#define cdddar(obj) cdr(cdr(cdr(car(obj))))
#define cddddr(obj) cdr(cdr(cdr(cdr(obj))))

object *eval(object *exp, object *env, unsigned long flags);

object *make_primitive_proc(
	object *(*fn)(struct object *arguments)) {
	object *obj;

	obj = alloc_object();
	obj->type = PRIMITIVE_PROC;
	obj->data.primitive_proc.fn = fn;
	return obj;
}

object *make_jmp_buf() {
	object *obj = alloc_object();
	obj->type = JMP_ENV;
	return obj;
}

object *make_exception(char *str) {
	object *obj = alloc_object();
	obj->type = EXCEPTION;
	obj->data.exception.msg = str;
	return obj;
}

char is_primitive_proc(object *obj) {
	return obj->type == PRIMITIVE_PROC;
}

object *is_null_proc(object *arguments) {
	return is_the_empty_list(car(arguments)) ? true : false;
}

object *is_boolean_proc(object *arguments) {
	return is_boolean(car(arguments)) ? true : false;
}

object *is_symbol_proc(object *arguments) {
	return is_symbol(car(arguments)) ? true : false;
}

object *is_integer_proc(object *arguments) {
	return is_fixnum(car(arguments)) ? true : false;
}

object *is_char_proc(object *arguments) {
	return is_character(car(arguments)) ? true : false;
}

object *is_string_proc(object *arguments) {
	return is_string(car(arguments)) ? true : false;
}

object *is_pair_proc(object *arguments) {
	return is_pair(car(arguments)) ? true : false;
}

char is_compound_proc(object *obj);

object *is_procedure_proc(object *arguments) {
	object *obj;
    
	obj = car(arguments);
	return (is_primitive_proc(obj) ||
		is_compound_proc(obj)) ?
                true :
                false;
}

object *char_to_integer_proc(object *arguments) {
	return make_fixnum((car(arguments))->data.character.value);
}

object *integer_to_char_proc(object *arguments) {
	return make_character((car(arguments))->data.fixnum.value);
}

object *number_to_string_proc(object *arguments) {
	char buffer[100];

	sprintf(buffer, "%lld", (car(arguments))->data.fixnum.value);
	return make_string(buffer);
}

object *string_to_number_proc(object *arguments) {
	return make_fixnum(atoi((car(arguments))->data.string.value));
}

object *symbol_to_string_proc(object *arguments) {
	return make_string((car(arguments))->data.symbol.value);
}

object *string_to_symbol_proc(object *arguments) {
	return make_symbol((car(arguments))->data.string.value);
}
void throw_error(char *msg);

object *add_proc(object *arguments) {
	double fresult = 0.0;
	int64_t result = 0;
	object *obj;
	char isdres = 0;

	printf("--> addproc ( ");
	print(arguments);
	printf(" )\n");
	/* printf("\n"); */
	/* printf("arg: ");     */
	while (!is_the_empty_list(arguments)) {
		obj = car(arguments);
		
			if (is_double(obj)) {
				if (!isdres) {
					isdres = 1;
					fresult = result;
				}
				
				fresult += obj->data.double_num.value;
			}
			else if(is_fixnum(obj))
				if (!isdres)
					result += obj->data.fixnum.value;
				else
					fresult += obj->data.fixnum.value;
			else
				throw_error("#<PROCEDURE +> error: not a number");

		arguments = cdr(arguments);
	}
	if (isdres)
		return make_double(fresult);
	else
		return make_fixnum(result);
}

object *sub_proc(object *arguments) {
	/* /\* long result; *\/ */
    
	/* /\* result = (car(arguments))->data.fixnum.value; *\/ */
	/* /\* while (!is_the_empty_list(arguments = cdr(arguments))) { *\/ */
	/* /\* 	result -= (car(arguments))->data.fixnum.value; *\/ */
	/* /\* } *\/ */
	/* /\* return make_fixnum(result); *\/ */
	/* double fresult = 0.0; */
	/* int64_t result = 0; */
	/* object *obj; */
	/* char isdres = 0; */

	/* printf("--> sub_proc ( "); */
	/* print(arguments); */
	/* printf(" )\n"); */
	/* /\* printf("\n"); *\/ */
	/* /\* printf("arg: ");     *\/ */
	/* while (!is_the_empty_list(arguments)) { */
	/* 	obj = car(arguments); */
		
	/* 	if (is_double(obj)) { */
	/* 		if (!isdres) { */
	/* 			isdres = 1; */
	/* 			fresult = result; */
	/* 		} */
				
	/* 		fresult -= obj->data.double_num.value; */
	/* 	} */
	/* 	else if(is_fixnum(obj)) */
	/* 		if (!isdres) */
	/* 			result -= obj->data.fixnum.value; */
	/* 		else */
	/* 			fresult -= obj->data.fixnum.value; */
	/* 	else */
	/* 		throw_error("#<PROCEDURE -> error: not a number"); */

	/* 	arguments = cdr(arguments); */
	/* } */
	/* if (isdres) */
	/* 	return make_double(fresult); */
	/* else */
	/* 	return make_fixnum(result); */

	object *obj1 = car(arguments);
	object *obj2 = car(cdr(arguments));
	char isdres = 0;

	printf("--> sub_proc ( ");
	print(arguments);
	printf(" )\n");

	if (!is_number(obj1) && !is_number(obj2))
		throw_error("#<PROCEDURE -> error: not a number");
	/* printf("\n"); */
	/* printf("arg: ");     */
	if (is_double(obj1) || is_double(obj2))
		isdres = 1;
	
	if (isdres)
	return make_double(
		(is_double(obj1) ? obj1->data.double_num.value : obj1->data.fixnum.value)
		-
		(is_double(obj2) ? obj2->data.double_num.value : obj2->data.fixnum.value)
		);

	else
		return make_fixnum(
			obj1->data.fixnum.value
			-
			obj2->data.fixnum.value
			);
}

object *mul_proc(object *arguments) {
	/* long result = 1; */
    
	/* while (!is_the_empty_list(arguments)) { */
	/* 	result *= (car(arguments))->data.fixnum.value; */
	/* 	arguments = cdr(arguments); */
	/* } */
	/* return make_fixnum(result); */
	double fresult = 1.0;
	int64_t result = 1;
	object *obj;
	char isdres = 0;

	printf("--> mul_proc ( ");
	print(arguments);
	printf(" )\n");
	/* printf("\n"); */
	/* printf("arg: ");     */
	while (!is_the_empty_list(arguments)) {
		obj = car(arguments);
		
		if (is_double(obj)) {
			if (!isdres) {
				isdres = 1;
				fresult = result;
			}
				
			fresult += obj->data.double_num.value;
		}
		else if(is_fixnum(obj))
			if (!isdres)
				result *= obj->data.fixnum.value;
			else
				fresult *= obj->data.fixnum.value;
		else
			throw_error("#<PROCEDURE *> error: not a number");

		arguments = cdr(arguments);
	}
	if (isdres)
		return make_double(fresult);
	else
		return make_fixnum(result);

}

object *div_proc(object *arguments) {
	/* long result = 1; */
    
	/* while (!is_the_empty_list(arguments)) { */
	/* 	result *= (car(arguments))->data.fixnum.value; */
	/* 	arguments = cdr(arguments); */
	/* } */
	/* return make_fixnum(result); */
	object *obj1 = car(arguments);
	object *obj2 = car(cdr(arguments));
	/* char isdres = 0; */

	printf("--> div_proc ( ");
	print(arguments);
	printf(" )\n");

	if (!is_number(obj1) && !is_number(obj2))
		throw_error("#<PROCEDURE /> error: not a number");
	/* printf("\n"); */
	/* printf("arg: ");     */
	/* if (is_double(obj1) || is_double(obj2)) */
	/* 	isdres = 1; */
	
	/* if (isdres){ */
		return make_double(
			(is_double(obj1) ? obj1->data.double_num.value : obj1->data.fixnum.value)
			/
			(is_double(obj2) ? obj2->data.double_num.value : obj2->data.fixnum.value)
			);

/* } */
	/* else */
	/* 	return make_fixnum( */
	/* 		obj1->data.fixnum.value */
	/* 		/ */
	/* 		obj2->data.fixnum.value */
	/* 		); */
}


object *quotient_proc(object *arguments) {
	return make_fixnum(
		((car(arguments) )->data.fixnum.value)/
		((cadr(arguments))->data.fixnum.value));
}

object *remainder_proc(object *arguments) {
	return make_fixnum(
		((car(arguments) )->data.fixnum.value)%
		((cadr(arguments))->data.fixnum.value));
}

object *is_number_equal_proc(object *arguments) {
	long value;
    
	value = (car(arguments))->data.fixnum.value;
	while (!is_the_empty_list(arguments = cdr(arguments))) {
		if (value != ((car(arguments))->data.fixnum.value)) {
			return false;
		}
	}
	return true;
}

object *is_less_than_proc(object *arguments) {
	long previous;
	long next;
    
	previous = (car(arguments))->data.fixnum.value;
	while (!is_the_empty_list(arguments = cdr(arguments))) {
		next = (car(arguments))->data.fixnum.value;
		if (previous < next) {
			previous = next;
		}
		else {
			return false;
		}
	}
	return true;
}

object *is_greater_than_proc(object *arguments) {
	long previous;
	long next;
    
	previous = (car(arguments))->data.fixnum.value;
	while (!is_the_empty_list(arguments = cdr(arguments))) {
		next = (car(arguments))->data.fixnum.value;
		if (previous > next) {
			previous = next;
		}
		else {
			return false;
		}
	}
	return true;
}

object *is_greater_than_or_equal_proc(object *arguments) {
	long previous;
	long next;
    
	previous = (car(arguments))->data.fixnum.value;
	while (!is_the_empty_list(arguments = cdr(arguments))) {
		next = (car(arguments))->data.fixnum.value;
		if (previous >= next) {
			previous = next;
		}
		else {
			return false;
		}
	}
	return true;
}

object *is_less_than_or_equal_proc(object *arguments) {
	long previous;
	long next;
    
	previous = (car(arguments))->data.fixnum.value;
	while (!is_the_empty_list(arguments = cdr(arguments))) {
		next = (car(arguments))->data.fixnum.value;
		if (previous <= next) {
			previous = next;
		}
		else {
			return false;
		}
	}
	return true;
}

object *is_number_proc(object *arguments) {
	return (car(arguments)->type == FIXNUM) ? true : false;
}


object *cons_proc(object *arguments) {
	return cons(car(arguments), cadr(arguments));
}

object *car_proc(object *arguments) {
	return caar(arguments);
}

object *cdr_proc(object *arguments) {
	return cdar(arguments);
}

object *set_car_proc(object *arguments) {
	set_car(car(arguments), cadr(arguments));
	return ok_symbol;
}

object *set_cdr_proc(object *arguments) {
	set_cdr(car(arguments), cadr(arguments));
	return ok_symbol;
}

object *list_proc(object *arguments) {
	return arguments;
}

object *iscons_proc(object *arguments) {
	return is_the_empty_list(car(arguments)) ? false : true;
}

object *pos_proc(object *arguments) {
	char *str = car (arguments)->data.string.value;
	int pos = car (cdr (arguments))->data.fixnum.value;

	char *res = malloc(sizeof(char[2]));
	res[0] = str[pos];
	res[1] = '\0';
	return make_string(res);
}

object *tlstr_proc(object *arguments) {
	char *str = car (arguments)->data.string.value;
	return make_string(str+1);
}

object *cn_proc(object *arguments) {
	char *str1 = car (arguments)->data.string.value;
	char *str2 = car (cdr (arguments))->data.string.value;

	int len1 = strlen(str1);
	int len2 = strlen(str2);

	char *res = malloc(len1+len2+1);
	strncpy(res, str1, len1);
	strncat(res, str2, len2);
	
	return make_string(res);
}

object *str_proc(object *arguments) {
	char *res="print_error";
	switch(car(arguments)->type) {
	case FIXNUM:
		res = malloc(20);
		sprintf(res, "^%lld^", car(arguments)->data.fixnum.value);
		break;
	case DOUBLE:
		res = malloc(20);
		sprintf(res, "^%lf^", car(arguments)->data.double_num.value);
		break;
	case SYMBOL:
		res = malloc(strlen(car(arguments)->data.symbol.value)+1);
		sprintf(res, "^%s^", car(arguments)->data.symbol.value);
		break;
	case STRING:
		res = malloc(strlen(car(arguments)->data.string.value)+1);
		sprintf(res, "^%s^", car(arguments)->data.string.value);
		break;
	case THE_EMPTY_LIST:
	case BOOLEAN:
	case ARGUMENT:
	case CHARACTER:
	case PAIR:
	case PRIMITIVE_PROC:
	case COMPOUND_PROC:
	case FREEZE:
	case JMP_ENV:
	case EXCEPTION:
	case VECTOR:
	case FILE_STREAM:
		break;
	}
	return make_string(res);
}

object *str_to_n_proc(object *arguments) {
	char sym = car(arguments)->data.string.value[0];

	return make_fixnum((int) sym);
}

object *n_to_str_proc(object *arguments) {
	int code = car(arguments)->data.fixnum.value;

	char res[2];
	res[0] = code;
	res[1] = '\0';
	return make_string(res);
}

char eq_vectors(object *obj1, object *obj2);
char eq_pairs(object *obj1, object *obj2);

object *is_eq_proc(object *arguments) {
	object *obj1;
	object *obj2;
    
	obj1 = car(arguments);
	obj2 = cadr(arguments);

	if (obj1->type != obj2->type && !(is_number(obj1) && is_number(obj2))) {
		return false;
	}

	switch (obj1->type) {
	case FIXNUM:
		if (obj2->type == FIXNUM)
			return (obj1->data.fixnum.value == 
					obj2->data.fixnum.value) ?
				true : false;
		else
			return ((double)obj1->data.fixnum.value == 
					obj2->data.double_num.value) ?
				true : false;
			
		break;
	case DOUBLE:
		if (obj2->type == DOUBLE)
			return (obj1->data.double_num.value == 
					obj2->data.double_num.value) ?
				true : false;
		else
			return (obj1->data.double_num.value == 
					(double) obj2->data.fixnum.value) ?
				true : false;
		break;

	case CHARACTER:
		return (obj1->data.character.value == 
			obj2->data.character.value) ?
                        true : false;
		break;
	case STRING:
		return (strcmp(obj1->data.string.value, 
			       obj2->data.string.value) == 0) ?
                        true : false;
	case BOOLEAN:
		return (obj1->data.boolean.value == 
				obj2->data.boolean.value) ?
			true : false;

	case VECTOR:
		return eq_vectors(obj1, obj2) ? true : false;
		break;

	case PAIR:
		return eq_pairs(obj1, obj2) ? true : false;
		break;

	case FILE_STREAM:
		return false;
		break;

        default:
		return (obj1 == obj2) ? true : false;
	}
}

object *and_proc(object *arguments) {
	return make_boolean(
		car(arguments)->data.boolean.value && 
		car(cdr(arguments))->data.boolean.value);
}

object *or_proc(object *arguments)
{
	return make_boolean(
		car(arguments)->data.boolean.value || 
		car(cdr(arguments))->data.boolean.value);
}

object *intern_proc(object *obj)
{
	return make_symbol(car(obj)->data.string.value);
}

object *error_to_string_proc(object *obj)
{
	return make_string(car(obj)->data.exception.msg);
}

object *type_proc(object *obj) {
	return car(obj);
}

object *make_vector(unsigned int length) {
	object *obj;
	int i;

	obj = alloc_object();
	obj->type = VECTOR;
	obj->data.vector.vec = malloc((length+1)*sizeof(struct object*));
	obj->data.vector.limit = length;

	for (i = 1; i <= length; i++)
		obj->data.vector.vec[i] = fail_symbol;
		
	obj->data.vector.vec[0] = (struct object*) 0;
	if (obj->data.vector.vec == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(1);
	}
	return obj;
}


object *absvector_proc(object *obj) {
	return make_vector(car(obj)->data.fixnum.value);
}

object *is_absvector_proc(object *obj) {
	return (car(obj)->type == VECTOR) ? true : false;
}

object *get_vec_elem_proc(object *obj) {
	object *vec = car(obj);
	unsigned long ind = car(cdr(obj))->data.fixnum.value;

	return vec->data.vector.vec[ind];
}


object *set_vec_elem_proc(object *obj) {
	object *vec = car(obj);
	unsigned long ind = car(cdr(obj))->data.fixnum.value;
	object *newval = car(cdr(cdr(obj)));
	vec->data.vector.vec[ind] = newval;
	return vec;
}

char eq_vectors(object *obj1, object *obj2) {
	int i;
	if (obj1->data.vector.limit != obj2->data.vector.limit)
		return 0;

	for (i = 1; i <= obj1->data.vector.limit; i++)
		if (false == is_eq_proc ( cons (obj1->data.vector.vec[i], cons(obj2->data.vector.vec[i], the_empty_list))))
			return 0;

	return 1;
}

char eq_pairs(object *obj1, object *obj2) {
	return (true == is_eq_proc(cons(car(obj1), cons(car(obj2),the_empty_list)))) &&
		(true == is_eq_proc(cons(cdr(obj1), cons(cdr(obj2), the_empty_list))));
}

void throw_error(char *msg) {
	char *exp_msg = strdup(msg);
	car(jmp_envs)->data.context.exception = make_exception(exp_msg);
	longjmp(car(jmp_envs)->data.context.jmp_env, 1);
}

object *open_proc(object *args) {
	object *type = car(args);
	object *path_obj = car(cdr(args));
	object *dir_obj = car(cdr(cdr(args)));
	FILE *fd;
	fstream_type fst;

	if (type != file_symbol) {
		return NULL;
	/* TODO: throw error */
	}

	char *fpath = path_obj->data.string.value;
	char rpath[MAX_FPATH];
	char *cur_dir = homedir_obj->data.string.value;
	
	sprintf(rpath, "%s/%s", cur_dir, fpath);
	
	if (dir_obj == in_symbol) {
		fd = fopen(rpath, "r");
		fst = FILE_IN;
	}
	else if (dir_obj == out_symbol) {
		fd = fopen(rpath, "w");
		fst = FILE_OUT;
	}
	else {
		return NULL;
		/* TODO: throw error */
	}

	if (fd == NULL) {
		char *err=malloc(256);
		sprintf(err, "#<PROCEDURE open (%s)>: %s", rpath, strerror(errno));
		throw_error(err);
	}

	return make_file_stream(fd, fpath, fst);
}

object *close_proc(object *args) {
	object *obj = car(args);
	fclose(obj->data.file_stream.fd);

	return the_empty_list;
}

object *read_byte_proc(object *args) {
	FILE *fd = car(args)->data.file_stream.fd;
	char sym;
	int ret = fread(&sym, 1, 1, fd);

	if (!ret) {
		if (feof(fd))
			sym = -1;
		if (ferror(fd))
			throw_error("file read error");
	}

	return make_fixnum(sym);
}

object *pr_proc(object *args) {
	char *str = car(args)->data.string.value;
	FILE *fd = car(cdr(args))->data.file_stream.fd;

	int len = strlen(str);
	int ret = fwrite(str, 1, len, fd);

	if (len != 0 && ret == 0) {
		if (ferror(fd)) {
			throw_error("file write error");
		}
	}
	
	return car(args);
}

#define _POSIX

int64_t timeval_to_usec( const struct timeval* tv )
{
	return( (int64_t)tv->tv_sec * 1000000 + tv->tv_usec ) ;
}

object *get_time_proc(object *args) {
/* #ifdef 	_POSIX */
	struct rusage usage;
	int ret;

	static int64_t rtime; /* TODO: make it reside in TLS */

	if (car(args) == run_symbol) {

		ret = getrusage(RUSAGE_SELF, &usage);

		if (ret == -1)
			throw_error("cannot get cpu resource usage");

		int64_t ntime =
			timeval_to_usec(&usage.ru_utime) +
			timeval_to_usec(&usage.ru_stime);
		
		int64_t diff = ntime-rtime;
		rtime = ntime;
		
		return make_double(diff/1000000.0);
	}
	else
		throw_error ("unknown input to get-time");

	return make_fixnum(0);
/* #endif */
	
/* #ifdef _WIN32 */
/* 	return make_fixnum(0); */
/* #endif */

}
object *parse(FILE *in);

object *load_proc(object *args) {
	char *path = car(args)->data.string.value;

	object *fstream = open_proc(cons(file_symbol, cons(make_string(path), cons(in_symbol, the_empty_list))));

	FILE *fp = fstream->data.file_stream.fd;

	printf("loading %s\n", fstream->data.file_stream.path);
	
	while (!feof(fp))
		println(eval(parse(fp), params_env, 0));

	fclose(fp);

	return ok_symbol;
}


/* object *eval_without_macros_proc(object *obj, object *env) { */
/* 	return eval(obj, env, 0); */
/* } */

object *lookup_variable_value(object *var, object *env);

/* object *value_proc(object *symbol, object *env) { */
/* 	return lookup_variable_value(symbol, env); */
/* } */

object *duplicate_symbol(object *sym) {
	object *obj;
	
	obj = alloc_object();
	obj->type = SYMBOL;
	obj->data.symbol.value = malloc(strlen(sym->data.symbol.value) + 1);
	obj->data.symbol.is_argument = 0;
	obj->data.symbol.is_func = 0;

	if (obj->data.symbol.value == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(1);
	}
	strcpy(obj->data.symbol.value, sym->data.symbol.value);

	return obj;
}

object *duplicate_symbol_list(object *list) {
	if (list == the_empty_list)
		return list;
	else
		return cons(duplicate_symbol(car(list)), duplicate_symbol_list(cdr(list)));
}

object *make_compound_proc(object *parameters, object *body,
                           object* env) {
	object *obj;

	/* object *param_list = duplicate_symbol_list(parameters); */

	/* object *tmp = param_list; */
	/* object *tmp = parameters; */
    
	obj = alloc_object();
	obj->type = COMPOUND_PROC;

	/* while (!is_empty_list(tmp)) { */
	/* 	car(tmp)->data.symbol.is_argument = 1; /\* <- HERE!! TODO: *\/ */
	/* 	tmp = cdr(tmp); */
	/* } */
	
	/* obj->data.compound_proc.parameters = param_list; */
	obj->data.compound_proc.parameters = parameters;
	obj->data.compound_proc.body = body;
	obj->data.compound_proc.env = env;
	return obj;
}

char is_compound_proc(object *obj) {
	return obj->type == COMPOUND_PROC;
}

object *enclosing_environment(object *env) {
	return cdr(env);
}

object *first_frame(object *env) {
	return car(env);
}

object *make_frame(object *variables, object *values) {
	return cons(variables, values);
}

object *frame_variables(object *frame) {
	return car(frame);
}

object *frame_values(object *frame) {
	return cdr(frame);
}

void add_binding_to_frame(object *var, object *val, 
                          object *frame) {
	set_car(frame, cons(var, car(frame)));
	set_cdr(frame, cons(val, cdr(frame)));
}

object *extend_environment(object *vars, object *vals,
                           object *base_env) {
	return cons(make_frame(vars, vals), base_env);
}

object *lookup_variable_value(object *var, object *env) {
	/* printf("-->in lookup_variable_value\n"); */
	object *frame;
	object *vars;
	object *vals;
	while (!is_the_empty_list(env)) {
		frame = first_frame(env);
		vars = frame_variables(frame);
		vals = frame_values(frame);
		while (!is_the_empty_list(vars)) {
			if (var == car(vars)) {
				return car(vals);
			}
			vars = cdr(vars);
			vals = cdr(vals);
		}
		env = enclosing_environment(env);
	}

	return NULL;


	/* char err[256]; */
	/* sprintf(err, "unbound variable: %s", var->data.symbol.value); */
	/* throw_error(err); */
	/* return NULL; */
	/* fprintf(stderr, "<--unbound variable: "); */
	
	/* print(var); */
	/* printf("\n"); */
	/* printf("env = "); */
	/* print(env); */
	/* printf("\n"); */
	/* exit(1); */
}

void set_variable_value(object *var, object *val, object *env) {
	object *frame;
	object *vars;
	object *vals;

	while (!is_the_empty_list(env)) {
		frame = first_frame(env);
		vars = frame_variables(frame);
		vals = frame_values(frame);
		while (!is_the_empty_list(vars)) {
			if (var == car(vars)) {
				set_car(vals, val);
				return;
			}
			vars = cdr(vars);
			vals = cdr(vals);
		}
		env = enclosing_environment(env);
	}
	/* env = extend_environment(var, val, env); */
	add_binding_to_frame(var, val, frame);
	/* printf("ext env: "); */
	/* print(env); */
	/* printf("\n"); */
}

void define_variable(object *var, object *val, object *env) {
	object *frame;
	object *vars;
	object *vals;
    
	frame = first_frame(env);    
	vars = frame_variables(frame);
	vals = frame_values(frame);

	while (!is_the_empty_list(vars)) {
		if (var == car(vars)) {
			set_car(vals, val);
			return;
		}
		vars = cdr(vars);
		vals = cdr(vals);
	}
	add_binding_to_frame(var, val, frame);
}

object *setup_environment(void) {
	object *initial_env;
    
	initial_env = extend_environment(
		the_empty_list,
		the_empty_list,
		the_empty_environment);
	return initial_env;
}

void init(void) {
	the_empty_list = alloc_object();
	the_empty_list->type = THE_EMPTY_LIST;
	jmp_envs = the_empty_list;

	false = alloc_object();
	false->type = BOOLEAN;
	false->data.boolean.value = 0;

	true = alloc_object();
	true->type = BOOLEAN;
	true->data.boolean.value = 1;
    
	symbol_table = the_empty_list;
	quote_symbol = make_symbol("quote");
	defun_symbol = make_symbol("defun");
	set_symbol = make_symbol("set");
	ok_symbol = make_symbol("ok");
	if_symbol = make_symbol("if");
	lambda_symbol = make_symbol("lambda");
	begin_symbol = make_symbol("begin");
	cond_symbol = make_symbol("cond");
	else_symbol = make_symbol("else");
	let_symbol = make_symbol("let");
	value_symbol = make_symbol("value");
	freeze_symbol = make_symbol("freeze");
	trap_error_symbol = make_symbol("trap-error");
	simple_error_symbol = make_symbol("simple-error");
	eval_without_macros_symbol = make_symbol("eval-without-macros");
	fail_symbol = make_symbol("fail!");

	in_symbol = make_symbol("in");
	out_symbol = make_symbol("out");
	file_symbol = make_symbol("file");

	run_symbol = make_symbol("run");
	
	the_empty_environment = the_empty_list;

	funcs_env = setup_environment();
	params_env = setup_environment();
	symbols_env = setup_environment();
	

#define add_procedure(scheme_name, c_name)              \
	define_variable(make_symbol(scheme_name),	\
			make_primitive_proc(c_name),	\
			funcs_env);

	add_procedure("null?"      , is_null_proc);
	add_procedure("boolean?"   , is_boolean_proc);
	add_procedure("symbol?"    , is_symbol_proc);
	add_procedure("integer?"   , is_integer_proc);
	/* add_procedure("char?"      , is_char_proc); */
	add_procedure("string?"    , is_string_proc);
	add_procedure("pair?"      , is_pair_proc);
	add_procedure("procedure?" , is_procedure_proc);
    
	add_procedure("char->integer" , char_to_integer_proc);
	add_procedure("integer->char" , integer_to_char_proc);
	add_procedure("number->string", number_to_string_proc);
	add_procedure("string->number", string_to_number_proc);
	add_procedure("symbol->string", symbol_to_string_proc);
	add_procedure("string->symbol", string_to_symbol_proc);
      
	add_procedure("+"        , add_proc);
	add_procedure("-"        , sub_proc);
	add_procedure("*"        , mul_proc);
	add_procedure("/"        , div_proc);
	/* add_procedure("quotient" , quotient_proc); */
	/* add_procedure("remainder", remainder_proc); */
	/* add_procedure("="        , is_number_equal_proc); */
	add_procedure("="        , is_eq_proc);
	add_procedure("<"        , is_less_than_proc);
	add_procedure(">"        , is_greater_than_proc);
	add_procedure("<="       , is_less_than_or_equal_proc);
	add_procedure(">="       , is_greater_than_or_equal_proc);
	add_procedure("number?"  , is_number_proc);
	

	add_procedure("and"      , and_proc);
	add_procedure("or"       , or_proc);

	add_procedure("cons"    , cons_proc);
	add_procedure("hd"     , car_proc);
	add_procedure("tl"     , cdr_proc);
	/* add_procedure("set-car!", set_car_proc); */
	/* add_procedure("set-cdr!", set_cdr_proc); */
	/* add_procedure("list"    , list_proc); */
	add_procedure("cons?", iscons_proc);

	/* add_procedure("eq?", is_eq_proc); */
	add_procedure("intern", intern_proc);

	add_procedure("error-to-string", error_to_string_proc);

	add_procedure("type", type_proc);

	add_procedure("pos", pos_proc);
	add_procedure("tlstr", tlstr_proc);
	add_procedure("cn", cn_proc);
	add_procedure("str", str_proc);
	add_procedure("string->n", str_to_n_proc);
	add_procedure("n->string", n_to_str_proc);

	add_procedure("absvector", absvector_proc);
	add_procedure("absvector?",is_absvector_proc);
	add_procedure("address->", set_vec_elem_proc);
	add_procedure("<-address", get_vec_elem_proc);

	add_procedure("open", open_proc);
	add_procedure("pr", pr_proc);
	add_procedure("close", close_proc);
	add_procedure("read-byte", read_byte_proc);

	add_procedure("get-time", get_time_proc);

	add_procedure("load-kl", load_proc);

	char *home_dir = malloc(256);
	getcwd(home_dir, 255); /* TODO: get system max path */

	homedir_obj = make_string(home_dir);
	
	symbols_env = extend_environment(
		cons(make_symbol("*home-directory*"), the_empty_list),
		cons(homedir_obj, the_empty_list),
		symbols_env);

#define IMP_LANG "ANSI C99"
#define IMP_PORT "native"
#define IMP_VERSION "0.1"
#define IMP_PORTERS "dca"
#define IMP_IMPL "glibc"

	symbols_env = extend_environment(
		cons(make_symbol("*language*"), the_empty_list),
		cons(make_string(IMP_LANG), the_empty_list),
		symbols_env);

	symbols_env = extend_environment(
		cons(make_symbol("*port*"), the_empty_list),
		cons(make_string(IMP_PORT), the_empty_list),
		symbols_env);

	symbols_env = extend_environment(
		cons(make_symbol("*version*"), the_empty_list),
		cons(make_string(IMP_VERSION), the_empty_list),
		symbols_env);

	symbols_env = extend_environment(
		cons(make_symbol("*porters*"), the_empty_list),
		cons(make_string(IMP_PORTERS), the_empty_list),
		symbols_env);

	symbols_env = extend_environment(
		cons(make_symbol("*implementation*"), the_empty_list),
		cons(make_string(IMP_IMPL), the_empty_list),
		symbols_env);

	

	/* add_procedure("eval-without-macros", (struct object * (*)(struct object *)) eval_without_macros_proc); */
	/* add_procedure("value", value_proc); */
}
 
/***************************** READ ******************************/

char is_delimiter(int c) {
	return isspace(c) || c == EOF ||
		c == '('   || c == ')' ||
		c == '"'   || c == ';';
}

char is_initial(int c) {
	return isalpha(c) ||
		c == '=' || c == '-' ||	c == '*' || c == '/' || c == '+' ||
		c == '_' || c == '?' || c == '$' || c == '!' || c == '@' ||
		c == '~' || c == '>' || c == '<' || c == '&' || c == '%' ||
		c == 39  || c == '#' || c == '`' || c == ';' || c == ':' ||
		c == '{' || c == '}' || c == '.';
}

int peek(FILE *in) {
	int c;

	c = getc(in);
	ungetc(c, in);
	return c;
}

void eat_whitespace(FILE *in) {
	int c;
    
	while ((c = getc(in)) != EOF) {
		if (isspace(c)) {
			continue;
		}
		/* else if (c == ';') { /\* comments are whitespace also *\/ */
		/* 	while (((c = getc(in)) != EOF) && (c != '\n')); */
		/* 	continue; */
		/* } */
		ungetc(c, in);
		break;
	}
}

void eat_expected_string(FILE *in, char *str) {
	int c;

	while (*str != '\0') {
		c = getc(in);
		if (c != *str) {
			fprintf(stderr, "unexpected character '%c'\n", c);
			exit(1);
		}
		str++;
	}
}

void peek_expected_delimiter(FILE *in) {
	if (!is_delimiter(peek(in))) {
		fprintf(stderr, "character not followed by delimiter\n");
		exit(1);
	}
}

object *read_character(FILE *in) {
	int c;

	c = getc(in);
	switch (c) {
        case EOF:
		fprintf(stderr, "incomplete character literal\n");
		exit(1);
        case 's':
		if (peek(in) == 'p') {
			eat_expected_string(in, "pace");
			peek_expected_delimiter(in);
			return make_character(' ');
		}
		break;
        case 'n':
		if (peek(in) == 'e') {
			eat_expected_string(in, "ewline");
			peek_expected_delimiter(in);
			return make_character('\n');
		}
		break;
	}
	peek_expected_delimiter(in);
	return make_character(c);
}

object *parse(FILE *in);

object *read_pair(FILE *in) {
	int c;
	object *car_obj;
	object *cdr_obj;
    
	eat_whitespace(in);
    
	c = getc(in);
	if (c == ')') { /* read the empty list */
		return the_empty_list;
	}
	ungetc(c, in);

	car_obj = parse(in);

	eat_whitespace(in);
    
	c = getc(in);    
	if (c == '.') { /* read improper list */
		c = peek(in);
		if (!is_delimiter(c)) {
			fprintf(stderr, "dot not followed by delimiter\n");
			exit(1);
		}
		cdr_obj = parse(in);
		eat_whitespace(in);
		c = getc(in);
		if (c != ')') {
			fprintf(stderr,
				"where was the trailing right paren?\n");
			exit(1);
		}
		return cons(car_obj, cdr_obj);
	}
	else { /* read list */
		ungetc(c, in);
		cdr_obj = read_pair(in);        
		return cons(car_obj, cdr_obj);
	}
}

object *parse(FILE *in) {
	int c;
	short sign = 1;
	int i;
	long num = 0;
	double decim = 0;
	double nd;
	float deco = 1.0;
	long expo = 0;
	short exp_sign = 1;
	double fnum;
	
	char is_decim = 0;
#define BUFFER_MAX 10000
	char buffer[BUFFER_MAX];

	eat_whitespace(in);

	c = getc(in);

	if (c == -1)
		return ok_symbol;

	if (c == '#') { /* read a boolean or character */
		c = getc(in);
		switch (c) {
		case 't':
			return true;
		case 'f':
			return false;
		case '\\':
			return read_character(in);
		default:
			fprintf(stderr,
				"unknown boolean or character literal\n");
			exit(1);
		}
	}
	else if (isdigit(c) || (c == '-' && (isdigit(peek(in))))) {
		/* read a fixnum */
		if (c == '-') {
			sign = -1;
		}
		else {
			ungetc(c, in);
		}
		while (isdigit(c = getc(in))) {
			num = (num * 10) + (c - '0');
		}
		if(c == '.') {
			if (is_decim == 1) {
				fprintf(stderr, "error reading floating point number\n");
				exit(1);
			}
				
			is_decim = 1;

			while (isdigit(c = getc(in))) {
				nd = (c - '0')/(deco *= 10.0);
				decim = decim + nd;
			}
		}
		if(c == 'e') {
			is_decim = 1;

			c = getc(in);
			if (c == '-') {
				exp_sign = -1;
			}
			else
				ungetc(c,in);

			while (isdigit(c = getc(in))) {
				expo = (expo * 10) + (c - '0');
			}
		}
		
		if (is_decim == 1)
			fnum = (num + decim) * sign * pow(10.0, exp_sign * (double)expo);
		else
			num *= sign;
		if (is_delimiter(c)) {
			ungetc(c, in);

			if (is_decim == 1)
				return make_double(fnum);
			return make_fixnum(num);
		}
		else {
			fprintf(stderr, "number not followed by delimiter\n");
			exit(1);
		}
	}
	else if (is_initial(c) ||
		 ((c == '+' || c == '-') &&
		  is_delimiter(peek(in)))) { /* read a symbol */
		i = 0;
		while (is_initial(c) || isdigit(c) ||
		       c == '+' || c == '-') {
			/* subtract 1 to save space for '\0' terminator */
			if (i < BUFFER_MAX - 1) {
				buffer[i++] = c;
			}
			else {
				fprintf(stderr, "symbol too long. "
					"Maximum length is %d\n", BUFFER_MAX);
				exit(1);
			}
			c = getc(in);
		}
		if (is_delimiter(c)) {
			buffer[i] = '\0';
			ungetc(c, in);
			return make_symbol(buffer);
		}
		else {
			fprintf(stderr, "symbol not followed by delimiter. "
				"Found '%c'\n", c);
			exit(1);
		}
	}
	else if (c == '"') { /* read a string */
		i = 0;
		while ((c = getc(in)) != '"') {
			if (c == '\\') {
				c = getc(in);
				if (c == 'n') {
					c = '\n';
				}
			}
			if (c == EOF) {
				fprintf(stderr, "non-terminated string literal\n");
				exit(1);
			}
			/* subtract 1 to save space for '\0' terminator */
			if (i < BUFFER_MAX - 1) {
				buffer[i++] = c;
			}
			else {
				fprintf(stderr, 
					"string too long. Maximum length is %d\n",
					BUFFER_MAX);
				exit(1);
			}
		}
		buffer[i] = '\0';
		return make_string(buffer);
	}
	else if (c == '(') { /* read the empty list or pair */
		return read_pair(in);
	}
	else if (c == '\'') { /* read quoted expression */
		return cons(quote_symbol, cons(parse(in), the_empty_list));
	}
	else {
		fprintf(stderr, "bad input. Unexpected '%c'\n", c);
		exit(1);
	}
	fprintf(stderr, "read illegal state\n");
	exit(1);
}

/*************************** EVALUATE ****************************/

char is_self_evaluating(object *exp, unsigned long flags) {
	return is_boolean(exp)   ||
		is_fixnum(exp)    ||
		is_double(exp) ||
		is_character(exp) ||
		(is_symbol(exp) && (!is_argument(exp)) && (exp->data.symbol.is_func==0)) ||
		is_string(exp) ||
		is_empty_list(exp);
}

char is_variable(object *expression) {
	return is_symbol(expression);
}

char is_tagged_list(object *expression, object *tag) {
	object *the_car;

	if (is_pair(expression)) {
		the_car = car(expression);
		return is_symbol(the_car) && (the_car == tag);
	}
	return 0;
}

char is_quoted(object *expression) {
	return is_tagged_list(expression, quote_symbol);
}

object *text_of_quotation(object *exp) {
	return cadr(exp);
}

char is_assignment(object *exp) {
	return is_tagged_list(exp, set_symbol);
}

object *assignment_variable(object *exp) {
	return car(cdr(exp));
}

object *assignment_value(object *exp) {
	return car(cdr(cdr(exp)));
}

char is_definition(object *exp) {
	/* printf("is definition\n"); */
	return is_tagged_list(exp, defun_symbol);
}

char is_value(object *exp) {
	return is_tagged_list(exp, value_symbol);
}

char is_freeze(object *exp) {
	return is_tagged_list(exp, freeze_symbol);
}

char is_thaw(object *exp) {
	return (exp->type == FREEZE);
}

char is_trap_error(object *exp) {
	return is_tagged_list(exp, trap_error_symbol);
}

char is_simple_error(object *exp) {
	return is_tagged_list(exp, simple_error_symbol);
}

char is_exception(object *exp) {
	return exp->type == EXCEPTION;
}

char is_eval_without_macros(object *exp) {
	return is_tagged_list(exp, eval_without_macros_symbol);
}

object *definition_variable(object *exp) {
	/* if (is_symbol(cadr(exp))) { */
	/* 	return cadr(exp); */
	/* } */
	/* else { */
	/* printf("in definition_variable\n"); */
	/* dump_object(exp, "in definition_variable: "); */
	return cadr(exp);
	/* } */
}

object *make_lambda(object *parameters, object *body);

object *definition_value(object *exp) {
	/* dump_object(exp, "in definition_value: \n"); */
	/* dump_object(caddr(exp), "args: "); */
	/* dump_object(cadddr(exp), "body: "); */
	
	return make_lambda(caddr(exp), cdddr(exp));
	/* printf("<-- definition_value\n"); */
}

object *make_if(object *predicate, object *consequent,
                object *alternative) {
	return cons(if_symbol,
		    cons(predicate,
			 cons(consequent,
			      cons(alternative, the_empty_list))));
}

char is_if(object *expression) {
	return is_tagged_list(expression, if_symbol);
}

object *if_predicate(object *exp) {
	return cadr(exp);
}

object *if_consequent(object *exp) {
	return caddr(exp);
}

object *if_alternative(object *exp) {
	if (is_the_empty_list(cdddr(exp))) {
		return false;
	}
	else {
		return cadddr(exp);
	}
}

object *make_lambda(object *parameters, object *body) {
	return cons(lambda_symbol, cons(parameters, body));
}

char is_lambda(object *exp) {
	return is_tagged_list(exp, lambda_symbol);
}

object *lambda_parameters(object *exp) {
	 /* shen lambdas have this form: */
	 /* (lambda s f) */
	 /* where s is symbol and f is any valid expression */
	 /* but since current defun code uses multi-parameter lambda */
	 /* i just handle it here */
	
	if (is_symbol(cadr(exp)))
		return cons(cadr(exp), the_empty_list);
	else
		return cadr(exp);
}

object *lambda_body(object *exp) {
	return cddr(exp);
}

object *make_begin(object *seq) {
	return cons(begin_symbol, seq);
}

char is_begin(object *exp) {
	return is_tagged_list(exp, begin_symbol);
}

object *begin_actions(object *exp) {
	return cdr(exp);
}

char is_last_exp(object *seq) {
	return is_the_empty_list(cdr(seq));
}

object *first_exp(object *seq) {
	return car(seq);
}

object *rest_exps(object *seq) {
	return cdr(seq);
}

char is_cond(object *exp) {
	return is_tagged_list(exp, cond_symbol);
}

object *cond_clauses(object *exp) {
	return cdr(exp);
}

object *cond_predicate(object *clause) {
	return car(clause);
}

object *cond_actions(object *clause) {
	return cdr(clause);
}

char is_cond_else_clause(object *clause) {
	return cond_predicate(clause) == else_symbol;
}

object *sequence_to_exp(object *seq) {
	if (is_the_empty_list(seq)) {
		return seq;
	}
	else if (is_last_exp(seq)) {
		return first_exp(seq);
	}
	else {
		return make_begin(seq);
	}
}

object *expand_clauses(object *clauses) {
	object *first;
	object *rest;
    
	if (is_the_empty_list(clauses)) {
		/* return false; */
		throw_error("cond clauses error");
	}
	else {
		first = car(clauses);
		rest  = cdr(clauses);
		printf("--> cond\n");
		printf("first clause: \n");
		println(first);
		printf("rest clauses: \n");
		println(rest);
			
		/* if (is_cond_else_clause(first)) { */
		/* 	if (is_the_empty_list(rest)) { */
		/* 		return sequence_to_exp(cond_actions(first)); */
		/* 	} */
		/* 	else { */
		/* 		fprintf(stderr, "else clause isn't last cond->if"); */
		/* 		exit(1); */
		/* 	} */
		/* } */
		/* else { */
			return make_if(cond_predicate(first),
				       sequence_to_exp(cond_actions(first)),
				       expand_clauses(rest));
		/* } */
	}
}

object *cond_to_if(object *exp) {
	return expand_clauses(cond_clauses(exp));
}

object *make_application(object *operator, object *operands) {
	return cons(operator, operands);
}

char is_application(object *exp) {
	return is_pair(exp);
}

object *operator(object *exp) {
	return car(exp);
}

object *operands(object *exp) {
	return cdr(exp);
}

char is_no_operands(object *ops) {
	return is_the_empty_list(ops);
}

object *first_operand(object *ops) {
	return car(ops);
}

object *rest_operands(object *ops) {
	return cdr(ops);
}

char is_let(object *exp) {
	return is_tagged_list(exp, let_symbol);
}

object *let_bindings(object *exp) {
	return cadr(exp);
}

object *let_body(object *exp) {
	return car(cdr(cdr(cdr(exp))));
}

object *binding_parameter(object *binding) {
	return cadr(binding);
}

object *binding_argument(object *binding) {
	return cadr(binding);
}

object *bindings_parameters(object *bindings) {
	return is_the_empty_list(bindings) ?
		the_empty_list :
		cons(binding_parameter(car(bindings)),
		     bindings_parameters(cdr(bindings)));
}

object *bindings_arguments(object *bindings) {
	return is_the_empty_list(bindings) ?
		the_empty_list :
		cons(binding_argument(car(bindings)),
		     bindings_arguments(cdr(bindings)));
}

object *let_parameter(object *exp) {
	return let_bindings(exp);
}

object *let_argument(object *exp) {
	return car(cdr(cdr(exp)));
}

object *let_to_application(object *exp) {
	return make_application(
		make_lambda(let_parameter(exp),
					cons(let_body(exp), the_empty_list)),
		cons(let_argument(exp),the_empty_list));
}

object *make_freeze(object *exp, object *env) {
	return make_compound_proc(the_empty_list,
							  exp,
							  env);
}

object *trap_func(object *exp) {
	return car(cdr(exp));
}

object *catch_func(object *exp) {
	return car(cdr(cdr(exp)));
}

		/* object *trap_func(object *exp) { */
		/* return car(cdr(exp); */
		/* 	   } */
			
		


object *list_of_values(object *exps, object *env, unsigned long flags) {
	if (is_no_operands(exps)) {
		return the_empty_list;
	}
	else {
		object *pcr = eval(first_operand(exps), env, flags);
		
		return cons(pcr,
			    list_of_values(rest_operands(exps), env, flags));
	}
}

object *eval_assignment(object *exp, object *env, unsigned long flags) {
	set_variable_value(assignment_variable(exp), 
			   eval(assignment_value(exp), env, flags),
			   symbols_env);
	/* printf("assgn env: "); */
	/* print(env); */
	/* printf("\n"); */

	/* long flags = EF_ARGUMENTS; */
	/* printf("EF is %lu\n", !(~flags & EF_ARGUMENTS)); */

	/* printf("in ev_asgn: symbols_env = "); */
	/* print(symbols_env); */
	/* printf("\n"); */

	return ok_symbol;
}

object *eval_definition(object *exp, object *env) {
	/* dump_object(exp, "in eval_definition: "); */
	/* dump_object(definition_value(exp), "def value:"); */
	/* printf("\n"); */
	
	define_variable(definition_variable(exp), 
			eval(definition_value(exp), env, EF_NULL),
			funcs_env);
	return definition_variable(exp);
}

object *eval(object *exp, object *env, unsigned long flags) {
	object *procedure;
	object *arguments;

	/* printf("-->eval\n"); */
tailcall:
	/* println(exp); */
	/* printf("\n"); */
	/* printf("-->tailcall\n"); */
	/* printf("env = "); */
	/* println(env); */
	
	
	if ((lookup_variable_value(exp, env) == NULL) && is_self_evaluating(exp, flags)) {
		/* printf("exp: "); */
		/* print(exp); */
		/* printf("\n is self-eval (flags = %lu )\n", flags); */
		return exp;
	}
	else if (is_variable(exp)) {
		/* printf("exp: "); */
		/* print(exp); */
		/* printf("\n is variable\n"); */
		object *obj = lookup_variable_value(exp, env);
		if (obj != NULL)
			return obj;
		else {
			char errstr[128];
			sprintf(errstr, "unbound variable: %s", exp->data.symbol.value);
			throw_error(errstr);
		}
	}
	else if (is_quoted(exp)) {
		return text_of_quotation(exp);
	}
	else if (is_assignment(exp)) {
		return eval_assignment(exp, env, flags);
	}
	else if (is_definition(exp)) {
		return eval_definition(exp, env);
	}
	else if (is_if(exp)) {
		exp = is_true(eval(if_predicate(exp), env, flags)) ?
			if_consequent(exp) :
			if_alternative(exp);
		goto tailcall;
	}
	else if (is_lambda(exp)) {
		return make_compound_proc(lambda_parameters(exp),
					  lambda_body(exp),
					  env);
	}
	else if (is_begin(exp)) {
		exp = begin_actions(exp);
		while (!is_last_exp(exp)) {
			eval(first_exp(exp), env, flags);
			exp = rest_exps(exp);
		}
		exp = first_exp(exp);
		goto tailcall;
	}
	else if (is_cond(exp)) {
		exp = cond_to_if(exp);
		goto tailcall;
	}
	else if (is_let(exp)) {
		exp = let_to_application(exp);
		goto tailcall;
	}
	else if (is_value(exp)) {
		object *obj = lookup_variable_value(binding_argument(exp), symbols_env);
		if (obj != NULL)
			return obj;
		else {
			char errstr[128];
			sprintf(errstr, "symbol not bound: %s", binding_argument(exp)->data.symbol.value);
			throw_error(errstr);
		}
	}
	else if (is_freeze(exp)) {
		return make_freeze(exp, env);
	}
	else if(is_trap_error(exp)) {
		object *new_jmp_buf = make_jmp_buf();
		jmp_envs = cons(new_jmp_buf, jmp_envs);

		if (setjmp(new_jmp_buf->data.context.jmp_env) != 0) {
			/* non local return here. exception happened */
			/* apply catch function to exception */
			jmp_envs = cdr(jmp_envs);
			return eval(cons(catch_func(exp), cons(new_jmp_buf->data.context.exception,the_empty_list)), env, 0);
		} else {
			/* normal return */
			/* call supervised function */
			return eval(trap_func(exp), env, 0);
		}
	}
	else if(is_simple_error(exp)) {
		if (jmp_envs == the_empty_list)
			return make_exception(car(cdr(exp))->data.string.value);
		else {
			car(jmp_envs)->data.context.exception = make_exception(car(cdr(exp))->data.string.value);
			longjmp(car(jmp_envs)->data.context.jmp_env, 1);
		}
	}
	else if(is_exception(exp)) {
		return make_string(exp->data.exception.msg);
	}

	else if(is_eval_without_macros(exp)) {
		/* return eval(car(cdr(exp)), env, 0); */
		/* exp = eval(car(cdr(exp)), env, 0); */
		return eval(car(cdr(car(cdr(exp)))), env, 0);
		/* goto tailcall; */
	}

	else if (is_application(exp)) {

		if (is_symbol(operator(exp))) {
			
			procedure = lookup_variable_value(operator(exp), env);

			if (procedure == NULL) {
				/* procedure is from global function env */
				procedure = lookup_variable_value(operator(exp), funcs_env);
				if (procedure == NULL) {
					/* procedure is not globally defined */
					char errstr[128];
					sprintf(errstr, "unknown procedure: %s", operator(exp)->data.symbol.value);
					throw_error(errstr);
				}
			}
			else if(is_symbol(procedure)) {
				/* procedure as a parameter */
				procedure = lookup_variable_value(procedure, funcs_env);
				if (procedure == NULL) {
					/* procedure is not globally defined */
					char errstr[128];
					sprintf(errstr, "unknown procedure: %s", operator(exp)->data.symbol.value);
					throw_error(errstr);
				}
			}
				
		}
		else 
			procedure = eval(operator(exp), env, flags);

		/* object *tmp = operands(exp); */

		/* while (!is_empty_list(tmp)) { */
		/* 	if (is_symbol(car(tmp))) { */
		/* 		car(tmp)->data.symbol.is_argument = 1; */
		/* 	} */
		/* 	tmp = cdr(tmp); */
		/* } */
		/* TODO: NEED AN EXTRA STACK SYMBOL TABLE FOR CLOSURE BINDINGS  */
		arguments = list_of_values(operands(exp), env, flags | EF_ARGUMENTS);

		/* tmp = operands(exp); */
		/* while (!is_empty_list(tmp)) { */
		/* 	if (is_symbol(car(tmp))) { */
		/* 		car(tmp)->data.symbol.is_argument = 0; */
		/* 	} */
		/* 	tmp = cdr(tmp); */
		/* } */


		/* printf("\n----------\nprocedure: "); */
		/* print(procedure); */
		/* printf("\n"); */

		/* printf("\n---------\nargs: "); */
		/* print(arguments); */
		/* printf("\n-----------\n"); */
	       
		if (is_primitive_proc(procedure)) {
			return (procedure->data.primitive_proc.fn)(arguments);
		}
		else if (is_compound_proc(procedure)) {
			env = extend_environment( 
				procedure->data.compound_proc.parameters,
				arguments,
				procedure->data.compound_proc.env);
			exp = make_begin(procedure->data.compound_proc.body);
			/* flags |= EF_ARGUMENTS; */
			/* printf("env: "); */
			/* print(env); */
			/* printf("\n--------------\n"); */
			goto tailcall;
		}
		else {
			fprintf(stderr, "unknown procedure type: ");
			print(procedure);
			printf("\npr type: %d\n", procedure->type);
			exit(1);
		}
	}
	else {
		fprintf(stderr, "cannot eval unknown expression type: \n");
		println(exp);

		exit(1);
	}
	fprintf(stderr, "eval illegal state\n");
	exit(1);
}

/**************************** PRINT ******************************/


void print_pair(object *pair) {
	object *car_obj;
	object *cdr_obj;
    
	car_obj = car(pair);
	cdr_obj = cdr(pair);
	print(car_obj);
	if (cdr_obj->type == PAIR) {
		printf(" ");
		print_pair(cdr_obj);
	}
	else if (cdr_obj->type == THE_EMPTY_LIST) {
		return;
	}
	else {
		printf(" . ");
		print(cdr_obj);
	}
}

void print(object *obj) {
	char c;
	char *str;
	int i;

	if (!obj)
		throw_error("print: NULL pointer!");
    
	switch (obj->type) {
	case THE_EMPTY_LIST:
		printf("()");
		break;
	case BOOLEAN:
		printf("#%c", is_false(obj) ? 'f' : 't');
		break;
	case SYMBOL:
		printf("%s", obj->data.symbol.value);
		break;
	case FIXNUM:
		printf("%lld", obj->data.fixnum.value);
		break;
	case DOUBLE:
		printf("%lf", obj->data.double_num.value);
		break;
	case CHARACTER:
		c = obj->data.character.value;
		printf("#\\");
		switch (c) {
                case '\n':
			printf("newline");
			break;
                case ' ':
			printf("space");
			break;
                default:
			putchar(c);
		}
		break;
	case STRING:
		str = obj->data.string.value;
		putchar('"');
		while (*str != '\0') {
			switch (*str) {
			case '\n':
				printf("\\n");
				break;
			case '\\':
				printf("\\\\");
				break;
			case '"':
				printf("\\\"");
				break;
			default:
				putchar(*str);
			}
			str++;
		}
		putchar('"');
		break;
	case PAIR:
		printf("(");
		print_pair(obj);
		printf(")");
		break;
	case PRIMITIVE_PROC:
		printf("#<PRIM PROCEDURE>");
		break;
	case COMPOUND_PROC:
		printf("#<PROCEDURE> :");
		printf("\nparams: \n");
		print(obj->data.compound_proc.parameters);
		printf("\nbody: \n");
		print(obj->data.compound_proc.body);
		break;
	case EXCEPTION:
		printf("#<exception>: %s", obj->data.exception.msg);
		break;

	case VECTOR:
		printf ("<");
		#define MAX_VEC_PRINT 10
		for (i = 1; i <= obj->data.vector.limit; i++) {
			printf(" ");
			print(obj->data.vector.vec[i]);
			if ( i > MAX_VEC_PRINT ) {
				printf("...(%d more elements)", obj->data.vector.limit - i);
				break;
			}
					
		}
		
		printf (" >");
		break;

	case FILE_STREAM:
		printf ("#<%s FILE STREAM \"%s\">",
				obj->data.file_stream.type == FILE_IN ? "INPUT":"OUTPUT",
				obj->data.file_stream.path
			);
		break;
			
        default:
			throw_error("cannot print unknown type");
		exit(1);
	}
}

void println(object *obj)
{
	print(obj);
	printf("\n");
}

/************************** DEBUG FACILITIES ******************************/
void dump_object(object *obj, char *str)
{
	/* print(obj); */
	switch(obj->type) {
	case SYMBOL:
		printf("%s %s", str, obj->data.symbol.value);
		break;
	case STRING:
		printf("%s %s", str, obj->data.string.value);
		break;
	case PAIR:
		printf("%s [ ", str);
		print_pair(obj);
		printf(" ]");
		break;
	default:
		printf("%s <unkn>", str);
	};
	printf("\n");
}


/***************************** REPL ******************************/

int main(void) {
	int first_it = 1;
	
	printf("Welcome to KL interpreter. "
	       "Use ctrl-c to exit.\n");

	init();

	object *new_jmp_buf = make_jmp_buf();
	jmp_envs = cons(new_jmp_buf, jmp_envs);

	while (1) {
		printf("> ");

		if (first_it) {
			first_it = 0;

			if (setjmp(new_jmp_buf->data.context.jmp_env) != 0) {
				/* non local return here. unhandled exception happened */

				printf("Unhandled exception: %s\n", new_jmp_buf->data.context.exception->data.exception.msg);
				continue;
			}
		}

		print(eval(parse(stdin), params_env, 0));
		printf("\n");
	}

	return 0;
}
