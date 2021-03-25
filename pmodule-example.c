#include <ctype.h>
#include <stddef.h>
#include <stdio.h>

#include <bt.h>
#include "ptrace/pmodule.h"

PMODULE_DEFINE("example_dynamic_pmodule", load, NULL, NULL);

/*
 * struct my_pair {
 *     int i;
 *     void *v;
 * };
 */
static void
pm_my_pair(ptrace_variable_t *pv, bt_variable_t *bv)
{
	bt_variable_t *i = NULL;
	bt_variable_t *v = NULL;
	char *buffer;

	i = bt_variable_member_value_get(bv, "i");
	v = bt_variable_member_value_get(bv, "v");

	if (i != NULL && v != NULL && asprintf(&buffer, "my_pair(%llu, %p)",
	    bt_variable_ull(i), (void *)bt_variable_ull(v)) != -1) {
		pmodule_variable_set_string(pv, buffer);
	}

	bt_variable_deinit(i);
	bt_variable_deinit(v);
	return;
}


/*
 * template <typename T>
 * struct my_type
 * {
 *     T templated;
 *     std::string obj_name;
 *     void *v; // points to templated
 * };
 */
static void
pm_my_type(ptrace_variable_t *pv, bt_variable_t *bv)
{
#define BUF_SIZE 512
	bt_variable_t *obj_name = NULL, *m_dataplus = NULL, *m_p = NULL, *v = NULL;
	bt_variable_t value;
	bt_error_t be;
	char *buffer, *name = NULL, *templated_on = NULL, *extra_info = NULL;
	ssize_t r;
	int asprintf_result;

	/* Extract members `obj_name` and `v` from `bv` (a `my_type` object) */
	obj_name = bt_variable_member_value_get(bv, "obj_name");
	v = bt_variable_member_value_get(bv, "v");
	if (obj_name == NULL || v == NULL)
		goto cleanup;

	/* Extract member `_M_dataplus` from `obj_name` */
	m_dataplus = bt_variable_member_value_get(obj_name, "_M_dataplus");
	if (m_dataplus == NULL)
		goto cleanup;

	/* Extract member `_M_p` from `_M_dataplus` */
	m_p = bt_variable_member_value_get(m_dataplus, "_M_p");
	if (m_p == NULL)
		goto cleanup;

	/* Extract the first template argument (as string) */
	if (pmodule_cpp_template_nth_argument(&templated_on, bv->type->name, 0) == false)
		goto cleanup;

	/*
	 * Special-case `my_pair`, for this example's sake.
	 *
	 * Usually, we'd access templated by the member `templated`, but in this
	 * case we want to show how to instantiate a variable of a known type
	 * having only its address. We will assume that member `v` points to 
	 * `templated`.
	 */
	if (strncmp(templated_on, "my_pair", 7) == 0) {
		bt_variable_t *i;
		unsigned long long addr;
		bt_variable_t my_pair;
		struct bt_type *type;
		ptrace_variable_t *pp;

		/* Get the type data for my_pair */
		type = pmodule_cpp_get_bt_type_from_name(bv->dwarf, templated_on);
		if (type == NULL)
			goto out;

		/* Read the value of `v` as unsigned long long */
		addr = bt_variable_ull(v);

		/*
		 * Dereference a variable of type `type` from address `addr`
		 * using `bv` as prototype into `my_pair`
		 */
		if (bt_variable_type_deref_at(&my_pair, bv, type, addr) == false)
			goto out;
		/* Evaluate the value of the variable */
		if (bt_variable_value(&my_pair) == false)
			goto out;

		/* Synthesize a member variable for this `my_pair` as an array member. */
		pp = pmodule_variable_create(templated_on, NULL, addr, 0, pv);
		if (pmodule_variable_value(pp, &my_pair, true) == false)
			goto out;

		pmodule_frame_add_variable(pmodule_variable_frame(pv), pp, true);
		pmodule_variable_dispatch(pp, &my_pair);


		/* Synthesize a member variable for this `my_pair` with custom name */
		pp = pmodule_ptrace_variable_create(pv,
		    pmodule_variable_depth(pv) + 1, -1);
		pmodule_variable_state_set(pv,
		    pmodule_variable_state(pv) | PTRACE_VARIABLE_S_SKIP);
		my_pair.name = "CustomName";

		if (pmodule_variable_value(pp, &my_pair, true) == false)
			goto out;

		pmodule_frame_add_variable(pmodule_variable_frame(pv), pp, true);
		pmodule_variable_dispatch(pp, &my_pair);


		/* Extrace member `i` from `my_pair` */
		i = bt_variable_member_value_get(&my_pair, "i");
		if (i == NULL)
			goto out;

		/*
		 * Prepare the `extra_info` string.
		 * In this example case it is the integer value of the `i` member.
		 */
		if (asprintf(&extra_info, "%lld", bt_variable_ll(i)) == -1)
			extra_info = NULL;
out:;
	}

	name = calloc(BUF_SIZE, 1);
	if (name == NULL)
		goto cleanup;

	/*
	 * Dereference the `m_p` member from the std::string internals. It points
	 * to the string data.
	 */
	if (bt_variable_deref(&value, m_p, &be) == false)
		goto cleanup;
	
	/* Evaluate the pointer to the string contents */
	if (bt_variable_value(&value) == false)
		goto cleanup;

	/* Copy data into the preallocated buffer */
	r = bt_variable_strncpy(name, &value, BUF_SIZE - 1);
	bt_variable_deinit(&value);
	if (r == -1)
		goto cleanup;

	/* Ensure all characters are printable (replace unprintable with ?s) */
	for (ssize_t k = 0; k < r; k++)
		if (isprint(name[k]) == false)
			name[k] = '?';

	/* Prepare the message, taking `extra_info` into account */
	if (extra_info == NULL) {
		asprintf_result = asprintf(&buffer, "my_type<%s>(name: %s)",
		    templated_on, name);
	} else {
		asprintf_result = asprintf(&buffer, "my_type<%s>(name: %s, i: %s)",
		    templated_on, name, extra_info);
	}

	/* Set the description into the ptrace variable */
	if (asprintf_result != -1) {
		pmodule_variable_set_string(pv, buffer);
	}

cleanup:
	free(extra_info);
	free(templated_on);
	free(name);
	return;
}

static void
pm_variable_dispatch(ptrace_variable_t *pv, bt_variable_t *bv)
{
	struct bt_type *type;
	const char *type_string;
	const char *base_type;

	type = bt_variable_bt_type(bv);
	if (type == NULL || type->die == NULL) {
		return;
	}
	type_string = bt_type_pp(type, BT_TYPE_LANG_C);
	base_type = bt_type_pp(bt_type_base(type), BT_TYPE_LANG_C);

	static struct {
		const char *string;
		bool prefix;
		void (*function)(ptrace_variable_t *, bt_variable_t *);
	} printer[] = {
		{
			.string = "struct(my_pair",
			.prefix = true,
			.function = pm_my_pair
		},
		{
			.string = "struct(my_type",
			.prefix = true,
			.function = pm_my_type
		},
	};

	for (size_t i = 0; i < sizeof(printer) / sizeof(*printer); i++) {
		bool m;

		if (printer[i].prefix == true) {
			m = strncmp(type_string, printer[i].string,
			    strlen(printer[i].string)) == 0;
		} else {
			m = strcmp(base_type, printer[i].string) == 0;
		}

		if (m == true) {
			printer[i].function(pv, bv);
			break;
		}
	}

	return;
}

static bool
load(pmodule_t *pm)
{

	return pmodule_variable_event_add(pm, NULL, pm_variable_dispatch);
}
