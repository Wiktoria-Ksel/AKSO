/*
 * Implementation of a dynamic stack structure storing 
 * uint64_t numbers and references to other stacks. 
 * Memory management is based on mark and sweep algorithm.
 * The library guarantees the consistency of data structures 
 * in the event of allocation errors.
 *
 * Author: Wiktoria Ksel
 */ 
#include <malloc.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <ctype.h>
#include <inttypes.h>
#include <time.h>

typedef enum {VALUE, RSTACK} node_kind_t;

struct rstack_t;

/*
 * Single element on the stack: 
 * value or reference to another stack.
 *
 * Field 'next' creates a linked list (stack structure).
 */
typedef struct node_t {
	node_kind_t kind;
	union element{
		uint64_t value;
		struct rstack_t* rs;
	} el;
	struct node_t* next; 
} node_t;

/*
 * Stack structure:
 * - top: start of the list of elements,
 * - reachable: can the stack be directly reached,
 * - visited: field used for identifying stacks that  
 *             cannot be reached (even indirectly),	
 * - next: pointer to the next stack in the list.
 */
typedef struct rstack_t {
	node_t* top;
	bool reachable;
	bool visited;
	struct rstack_t* next; 
} rstack_t;

typedef struct result_t {
	bool flag;
	uint64_t value;
} result_t;

/*
 * List of allocated stacks.
 * Required for the sweep phase of garbage collection.
 */
typedef struct list {
	rstack_t* rs;
} list;

static list stacks = {NULL}; 

/*
 * Creates new rstack and returns a pointer to it 
 * or nullptr in case of failure
 * (in which case it sets errno to ENOMEM).
 */ 
rstack_t* rstack_new() {
	rstack_t* rs = (rstack_t*)malloc(sizeof(rstack_t));
	if (rs == NULL) {
		errno = ENOMEM;
		return NULL;
	}

	rs->top = NULL;
	rs->reachable = true;
	rs->visited = false;
	rs->next = stacks.rs;
	stacks.rs = rs;

	return rs;
}

/*
 * Sets visited = true for every stack
 * that can be directly or indirectly reached
 * starting from rs.
 * Assumes that rs can be reached.
 */ 
void set_visited(rstack_t* rs) {
	rs->visited = true;
	node_t* node = rs->top;
	rstack_t* next_rs;

	while (node != NULL) {
		if (node->kind == RSTACK) {
			next_rs = node->el.rs;
			if (!next_rs->visited) {
				set_visited(next_rs);
			} 
		}
		node = node->next;
	}
}

/*
 * Permanently deletes the next element in list 'stacks'.
 */
void permanent_delete(rstack_t* ptr) {
	rstack_t* cur = ptr->next;
	node_t* node = cur->top;
	node_t* next_node;

	/* Deletes the elements of the stack. */
	while (node != NULL) {
		next_node = node->next;
		free(node);
		node = next_node;
	}

	/* Frees the stack and deletes it from the list. */
	ptr->next = cur->next;
	free(cur);
}

/*
 * Deletes the stack or does nothing if the argument is nullptr.
 */ 
void rstack_delete(rstack_t* rs) {
	if (rs == NULL) {
		return;
	}

	rs->reachable = false;

	/* Identifies the reachable stacks. */
	rstack_t* ptr = stacks.rs;
	while (ptr != NULL) {
		if (ptr->reachable && !ptr->visited) {
			set_visited(ptr);
		}
		ptr = ptr->next;
	}

	/* Deletes the unreachable stacks. */
	rstack_t atr;
	atr.next = stacks.rs;
	ptr = &atr;
	rstack_t* cur = ptr->next;

	while (cur != NULL) {
		if (!cur->visited) {
			cur = cur->next;
			permanent_delete(ptr);
		}
		else {
			cur->visited = false;
			cur = cur->next;
			ptr = ptr->next;
		}
	}
	stacks.rs = atr.next;
}

/*
 * Pushes the given value to the stack.
 * Returns -1 in case of failure or if rs is a nullptr.
 * (Sets errno to ENOMEM or EINVAL respectively.)
 * Otherwise returns 0.
 */ 
int rstack_push_value(rstack_t *rs, uint64_t val) {
	if (rs == NULL) {
		errno = EINVAL;
		return -1;
	}

	node_t* elem = (node_t*)malloc(sizeof(node_t));
	if (elem == NULL) {
		errno = ENOMEM;
		return -1;
	}

	elem->kind = VALUE;
	(elem->el).value = val;
	elem->next = rs->top;
	rs->top = elem;

	return 0;
}

/*
 * Pushes a stack reference to a given stack.
 * Returns -1 in case of failure or if rs is a nullptr.
 * Otherwise returns 0.
 */ 
int rstack_push_rstack(rstack_t* rs1, rstack_t* rs2) {
	if (rs1 == NULL || rs2 == NULL) {
		errno = EINVAL;
		return -1;
	}
	node_t* elem = (node_t*)malloc(sizeof(node_t));
	if (elem == NULL) {
		errno = ENOMEM;
		return -1;
	}

	elem->kind = RSTACK;
	(elem->el).rs = rs2;
	elem->next = rs1->top;
	rs1->top = elem;

	return 0;
}

/*
 * Deletes the top element of the given stack.
 * If the stack is empty, the function does nothing.
 */ 
void rstack_pop(rstack_t* rs) {
	if (rs != NULL) {
		node_t* tmp = rs->top;  
		if (tmp != NULL) {
			node_t* new_top = tmp->next;
			free(tmp);
			rs->top = new_top;
		}
	}	
}

/*
 * Finds the element closest to the top of a given stack.
 * Returns a flag and a value.
 *
 * Flag = true iff there is a value in the stack.
 * In that case the value field is equal to the found element.
 * Otherwise flag is set to false.
 */ 
result_t _rstack_front(rstack_t* rs) {
	result_t res;
	if (rs == NULL || rs->visited) {
		res.flag = false;
		return res;
	}

	rs->visited = true;
	node_t* node = rs->top;

	while (node != NULL) {
		if (node->kind == VALUE) {
			rs->visited = false;
			res.flag = true;
			res.value = (node->el).value;
			return res;
		}
		else if (!(node->el.rs)->visited){
			res = _rstack_front(node->el.rs);	
			if (res.flag == true) {
				return res;
			}
		}
		node = node->next;
	}

	res.flag = false;
	return res;
}

/*
 * Calls _rstack_front and then clears 'visited' field.
 */ 
result_t rstack_front(rstack_t* rs) {
	result_t res = _rstack_front(rs);

	rstack_t* cur = stacks.rs;
	while (cur != NULL) {
		cur->visited = false;
		cur = cur->next;
	}

	return res;
}

/*
 * Return true iff there is a value in the stack.
 */ 
bool rstack_empty(rstack_t* rs) {
	return !rstack_front(rs).flag;
}

/* 
 * Reads a number from the file starting with 'c'.
 */
result_t read_number(int* c, FILE* f) {
	result_t res;
	char buf[32];
	int i = 0;
	int has_zero = 0;

	while (*c == 0) {
		has_zero = 1;
		*c = fgetc(f);
	}
	while (*c != EOF && isdigit(*c) && i < 31) {
		buf[i++] = *c;
		*c = fgetc(f);
	}

	if (i == 31 && isdigit(*c)) {  /* The value is too big. */
		fclose(f);
		res.flag = false;
		return res;
	}
	buf[i] = '\0';
	errno = 0;
	char* end;
	uint64_t value = strtoumax(buf, &end, 10);

	if (has_zero > 0 && i == 0) {
		value = 0;
	}

	if (errno == ERANGE || value > UINT64_MAX) {
		fclose(f);
		res.flag = false;
		return res;
	}

	res.value = value;
	res.flag = true;
	return res;
}


/*
 * Creates a stack from the numbers given in a file.
 *
 * The numbers in the file are given in base 10 notation. 
 * They are separated by whitespace. 
 * There can be more than one whitespace character between two numbers. 
 * There can be any number of whitespace characters at the beginning and end of the file. 
 * The function thoroughly checks the file's contents for validity.
 *
 * Returns pointer to the stack or nullptr in case of failure 
 * or when path is a nullptr
 * and sets errno to EINVAL.
 */ 
rstack_t* rstack_read(char const* path) {
	if (path == NULL) {
		errno = EINVAL;
		return NULL;
	}

	FILE* f = fopen(path, "r"); 
	if (f == NULL) {
		return NULL;	/* Errno is already set by fopen. */
	}

	rstack_t* rs = rstack_new();
	if (rs == NULL) {
		fclose(f);
		return NULL;
	}

	int c;
	while ((c = fgetc(f)) != EOF && isspace(c));
	while (c != EOF) {
		if (!isdigit(c)) {
			fclose(f);
			rstack_delete(rs);
			errno = EINVAL;
			return NULL;
		}

		result_t res = read_number(&c, f);
		if (!res.flag) {
			rstack_delete(rs);
			errno = EINVAL;
			return NULL;
		}
		uint64_t value = res.value;

		if (c != EOF && !isspace(c)) {
			fclose(f);
			rstack_delete(rs);
			errno = EINVAL;
			return NULL;
		}

		if (rstack_push_value(rs, value) != 0) {
			fclose(f);
			rstack_delete(rs);
			return NULL; 	/* Errno is already set by rstack_push_value. */
		}

		while (c != EOF && isspace(c)) {
			c = fgetc(f);
		}

	}

	fclose(f);
	return rs;
}

/*
 * Complementary function that allows rstack_write
 * to write the values of a given stack from bottom to top.
 *
 * - node -> Node from which the function starts its search.
 * - cycle -> True iff it found a cycle on its path.
 *
 * The functions returns -1 if path or rs is a nullptr
 * or in case of failure.
 * Otherwise it returns true.
 */ 
int recursive_write(node_t* node, FILE* f, bool* cycle) {
	if (node == NULL) {
		return 0;
	}

	bool cycle_rec = false;
	int res = recursive_write(node->next, f, &cycle_rec);

	if (res != 0) {
		return res;
	}

	if (cycle_rec == false) {
		if (node->kind == VALUE) {
			if (fprintf(f, "%" PRIu64 "\n", (node->el).value) < 0) {
				return -1;    
			}
		}
		else {
			rstack_t* rs = node->el.rs;
			if (!rs->visited) {
				rs->visited = true;
				res = recursive_write(rs->top, f, &cycle_rec);
				*cycle = cycle_rec;
				rs->visited = false;
			}
			else {
				*cycle = true;
			}
		}
	}
	else {
		*cycle = true;
	}

	return 0;
}

/*
 * Puts the values of a given stack in a file.
 * Each number is written on a separate line in base-10 representation without leading zeros. 
 * Each line ends with a newline character. 
 * There are no other whitespace characters. 
 * If a cycle is detected during writing, writing is aborted.
 *
 * The functions returns -1 if path or rs is a nullptr
 * or in case of failure and sets errno to EINVAL.
 * Otherwise it returns true.
 */ 
int rstack_write(char const *path, rstack_t* rs) {
	if (path == NULL) {
		errno = EINVAL;	// ...?
		return -1;
	}
	if (rs == NULL) {
		errno = EINVAL;
		return -1;
	}

	FILE* f = fopen(path, "w");
	if (f == NULL) {
		return -1;
	}

	rs->visited = true;
	bool cycle = false;
	int res = recursive_write(rs->top, f, &cycle);
	rs->visited = false;

	int saved_errno = errno;
	int close_res = fclose(f);
	if (res != 0) {
		errno = saved_errno;
		return -1;
	}
	if (close_res != 0) {
		return -1;
	}

	return 0;
}

