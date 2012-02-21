/*
 * Bootstrap Scheme - a quick and very dirty Scheme interpreter.
 * Copyright (C) 2010 Peter Michaux (http://peter.michaux.ca/)
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public
 * License version 3 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License version 3 for more details.
 *
 * You should have received a copy of the GNU Affero General Public
 * License version 3 along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

/**************************** MODEL ******************************/

typedef enum {THE_EMPTY_LIST, BOOLEAN, SYMBOL, FIXNUM,
              CHARACTER, STRING, PAIR, PRIMITIVE_PROC,
              COMPOUND_PROC} object_type;

typedef struct object {
	object_type type;
	union {
		struct {
			char value;
		} boolean;
		struct {
			char *value;
		} symbol;
		struct {
			long value;
		} fixnum;
		struct {
			char value;
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
	} data;
} object;

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
object *the_empty_environment;
object *the_global_environment;

object *cons(object *car, object *cdr);
object *car(object *pair);
object *cdr(object *pair);

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
    
	/* search for they symbol in the symbol table */
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
	if (obj->data.symbol.value == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(1);
	}
	strcpy(obj->data.symbol.value, value);
	symbol_table = cons(obj, symbol_table);
	return obj;
}

char is_symbol(object *obj) {
	return obj->type == SYMBOL;
}

object *make_fixnum(long value) {
	object *obj;

	obj = alloc_object();
	obj->type = FIXNUM;
	obj->data.fixnum.value = value;
	return obj;
}

char is_fixnum(object *obj) {
	return obj->type == FIXNUM;
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

char is_string(object *obj) {
	return obj->type == STRING;
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

object *make_primitive_proc(
	object *(*fn)(struct object *arguments)) {
	object *obj;

	obj = alloc_object();
	obj->type = PRIMITIVE_PROC;
	obj->data.primitive_proc.fn = fn;
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

	sprintf(buffer, "%ld", (car(arguments))->data.fixnum.value);
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

object *add_proc(object *arguments) {
	long result = 0;
    
	while (!is_the_empty_list(arguments)) {
		result += (car(arguments))->data.fixnum.value;
		arguments = cdr(arguments);
	}
	return make_fixnum(result);
}

object *sub_proc(object *arguments) {
	long result;
    
	result = (car(arguments))->data.fixnum.value;
	while (!is_the_empty_list(arguments = cdr(arguments))) {
		result -= (car(arguments))->data.fixnum.value;
	}
	return make_fixnum(result);
}

object *mul_proc(object *arguments) {
	long result = 1;
    
	while (!is_the_empty_list(arguments)) {
		result *= (car(arguments))->data.fixnum.value;
		arguments = cdr(arguments);
	}
	return make_fixnum(result);
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

object *is_eq_proc(object *arguments) {
	object *obj1;
	object *obj2;
    
	obj1 = car(arguments);
	obj2 = cadr(arguments);
    
	if (obj1->type != obj2->type) {
		return false;
	}
	switch (obj1->type) {
        case FIXNUM:
		return (obj1->data.fixnum.value == 
			obj2->data.fixnum.value) ?
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
		break;
        default:
		return (obj1 == obj2) ? true : false;
	}
}

object *make_compound_proc(object *parameters, object *body,
                           object* env) {
	object *obj;
    
	obj = alloc_object();
	obj->type = COMPOUND_PROC;
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
	fprintf(stderr, "unbound variable\n");
	exit(1);
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
	fprintf(stderr, "unbound variable\n");
	exit(1);
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

	false = alloc_object();
	false->type = BOOLEAN;
	false->data.boolean.value = 0;

	true = alloc_object();
	true->type = BOOLEAN;
	true->data.boolean.value = 1;
    
	symbol_table = the_empty_list;
	quote_symbol = make_symbol("quote");
	defun_symbol = make_symbol("defun");
	set_symbol = make_symbol("set!");
	ok_symbol = make_symbol("ok");
	if_symbol = make_symbol("if");
	lambda_symbol = make_symbol("lambda");
	begin_symbol = make_symbol("begin");
	cond_symbol = make_symbol("cond");
	else_symbol = make_symbol("else");
	let_symbol = make_symbol("let");
    
	the_empty_environment = the_empty_list;

	the_global_environment = setup_environment();

#define add_procedure(scheme_name, c_name)              \
	define_variable(make_symbol(scheme_name),	\
			make_primitive_proc(c_name),	\
			the_global_environment);

	add_procedure("null?"      , is_null_proc);
	add_procedure("boolean?"   , is_boolean_proc);
	add_procedure("symbol?"    , is_symbol_proc);
	add_procedure("integer?"   , is_integer_proc);
	add_procedure("char?"      , is_char_proc);
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
	add_procedure("quotient" , quotient_proc);
	add_procedure("remainder", remainder_proc);
	add_procedure("="        , is_number_equal_proc);
	add_procedure("<"        , is_less_than_proc);
	add_procedure(">"        , is_greater_than_proc);

	add_procedure("cons"    , cons_proc);
	add_procedure("car"     , car_proc);
	add_procedure("cdr"     , cdr_proc);
	add_procedure("set-car!", set_car_proc);
	add_procedure("set-cdr!", set_cdr_proc);
	add_procedure("list"    , list_proc);

	add_procedure("eq?", is_eq_proc);
}

/***************************** READ ******************************/

char is_delimiter(int c) {
	return isspace(c) || c == EOF ||
		c == '('   || c == ')' ||
		c == '"'   || c == ';';
}

char is_initial(int c) {
	return isalpha(c) || c == '*' || c == '/' || c == '>' ||
		c == '<' || c == '=' || c == '?' || c == '!';
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
		else if (c == ';') { /* comments are whitespace also */
			while (((c = getc(in)) != EOF) && (c != '\n'));
			continue;
		}
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

object *read(FILE *in);

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

	car_obj = read(in);

	eat_whitespace(in);
    
	c = getc(in);    
	if (c == '.') { /* read improper list */
		c = peek(in);
		if (!is_delimiter(c)) {
			fprintf(stderr, "dot not followed by delimiter\n");
			exit(1);
		}
		cdr_obj = read(in);
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

object *read(FILE *in) {
	int c;
	short sign = 1;
	int i;
	long num = 0;
#define BUFFER_MAX 1000
	char buffer[BUFFER_MAX];

	eat_whitespace(in);

	c = getc(in);    

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
		num *= sign;
		if (is_delimiter(c)) {
			ungetc(c, in);
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
		return cons(quote_symbol, cons(read(in), the_empty_list));
	}
	else {
		fprintf(stderr, "bad input. Unexpected '%c'\n", c);
		exit(1);
	}
	fprintf(stderr, "read illegal state\n");
	exit(1);
}

/*************************** EVALUATE ****************************/

char is_self_evaluating(object *exp) {
	return is_boolean(exp)   ||
		is_fixnum(exp)    ||
		is_character(exp) ||
		is_string(exp);
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
	return is_tagged_list(exp, defun_symbol);
}

object *definition_variable(object *exp) {
	if (is_symbol(cadr(exp))) {
		return cadr(exp);
	}
	else {
		return caadr(exp);
	}
}

object *make_lambda(object *parameters, object *body);

object *definition_value(object *exp) {
	if (is_symbol(cadr(exp))) {
		return caddr(exp);
	}
	else {
		return make_lambda(cdadr(exp), cddr(exp));
	}
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
		return false;
	}
	else {
		first = car(clauses);
		rest  = cdr(clauses);
		if (is_cond_else_clause(first)) {
			if (is_the_empty_list(rest)) {
				return sequence_to_exp(cond_actions(first));
			}
			else {
				fprintf(stderr, "else clause isn't last cond->if");
				exit(1);
			}
		}
		else {
			return make_if(cond_predicate(first),
				       sequence_to_exp(cond_actions(first)),
				       expand_clauses(rest));
		}
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
	return cddr(exp);
}

object *binding_parameter(object *binding) {
	return car(binding);
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

object *let_parameters(object *exp) {
	return bindings_parameters(let_bindings(exp));
}

object *let_arguments(object *exp) {
	return bindings_arguments(let_bindings(exp));
}

object *let_to_application(object *exp) {
	return make_application(
		make_lambda(let_parameters(exp),
			    let_body(exp)),
		let_arguments(exp));
}

object *eval(object *exp, object *env);

object *list_of_values(object *exps, object *env) {
	if (is_no_operands(exps)) {
		return the_empty_list;
	}
	else {
		return cons(eval(first_operand(exps), env),
			    list_of_values(rest_operands(exps), env));
	}
}

object *eval_assignment(object *exp, object *env) {
	set_variable_value(assignment_variable(exp), 
			   eval(assignment_value(exp), env),
			   env);
	return ok_symbol;
}

object *eval_definition(object *exp, object *env) {
	define_variable(definition_variable(exp), 
			eval(definition_value(exp), env),
			env);
	return ok_symbol;
}

object *eval(object *exp, object *env) {
	object *procedure;
	object *arguments;

tailcall:
	if (is_self_evaluating(exp)) {
		return exp;
	}
	else if (is_variable(exp)) {
		return lookup_variable_value(exp, env);
	}
	else if (is_quoted(exp)) {
		return text_of_quotation(exp);
	}
	else if (is_assignment(exp)) {
		return eval_assignment(exp, env);
	}
	else if (is_definition(exp)) {
		return eval_definition(exp, env);
	}
	else if (is_if(exp)) {
		exp = is_true(eval(if_predicate(exp), env)) ?
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
			eval(first_exp(exp), env);
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
	else if (is_application(exp)) {
		procedure = eval(operator(exp), env);
		arguments = list_of_values(operands(exp), env);
		if (is_primitive_proc(procedure)) {
			return (procedure->data.primitive_proc.fn)(arguments);
		}
		else if (is_compound_proc(procedure)) {
			env = extend_environment( 
				procedure->data.compound_proc.parameters,
				arguments,
				procedure->data.compound_proc.env);
			exp = make_begin(procedure->data.compound_proc.body);
			goto tailcall;
		}
		else {
			fprintf(stderr, "unknown procedure type\n");
			exit(1);
		}
	}
	else {
		fprintf(stderr, "cannot eval unknown expression type\n");
		exit(1);
	}
	fprintf(stderr, "eval illegal state\n");
	exit(1);
}

/**************************** PRINT ******************************/

void write(object *obj);

void write_pair(object *pair) {
	object *car_obj;
	object *cdr_obj;
    
	car_obj = car(pair);
	cdr_obj = cdr(pair);
	write(car_obj);
	if (cdr_obj->type == PAIR) {
		printf(" ");
		write_pair(cdr_obj);
	}
	else if (cdr_obj->type == THE_EMPTY_LIST) {
		return;
	}
	else {
		printf(" . ");
		write(cdr_obj);
	}
}

void write(object *obj) {
	char c;
	char *str;
    
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
		printf("%ld", obj->data.fixnum.value);
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
		write_pair(obj);
		printf(")");
		break;
        case PRIMITIVE_PROC:
        case COMPOUND_PROC:
		printf("#<procedure>");
		break;
        default:
		fprintf(stderr, "cannot write unknown type\n");
		exit(1);
	}
}

/***************************** REPL ******************************/

int main(void) {

	printf("Welcome to Bootstrap Shen. "
	       "Use ctrl-c to exit.\n");

	init();

	while (1) {
		printf("> ");
		write(eval(read(stdin), the_global_environment));
		printf("\n");
	}

	return 0;
}

/**************************** MUSIC *******************************

Slipknot, Neil Young, Pearl Jam, The Dead Weather,
Dave Matthews Band, Alice in Chains, White Zombie, Blind Melon,
Priestess, Puscifer, Bob Dylan, Them Crooked Vultures,
Black Sabbath, Pantera, Tool, ZZ Top, Queens of the Stone Age,
Raised Fist, Rage Against the Machine, Primus, Black Label Society,
The Offspring

*/
