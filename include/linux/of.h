#ifndef _LINUX_OF_H
#define _LINUX_OF_H
/*
 * Definitions for talking to the Open Firmware PROM on
 * Power Macintosh and other computers.
 *
 * Copyright (C) 1996-2005 Paul Mackerras.
 *
 * Updates for PPC64 by Peter Bergner & David Engebretsen, IBM Corp.
 * Updates for SPARC64 by David S. Miller
 * Derived from PowerPC and Sparc prom.h files by Stephen Rothwell, IBM Corp.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */
#include <linux/types.h>
#include <linux/bitops.h>
#include <linux/errno.h>
#include <linux/kref.h>
#include <linux/mod_devicetable.h>
#include <linux/spinlock.h>
#include <linux/topology.h>
#include <linux/notifier.h>
#include <linux/i2c.h>

#include <asm/byteorder.h>
#include <asm/errno.h>

typedef u32 phandle;
typedef u32 ihandle;

struct property {
	char	*name;
	int	length;
	void	*value;
	struct property *next;
	unsigned long _flags;
	unsigned int unique_id;
};

#if defined(CONFIG_SPARC)
struct of_irq_controller;
#endif

struct device_node {
	const char *name;
	const char *type;
	phandle phandle;
	const char *full_name;

	struct	property *properties;
	struct	property *deadprops;	/* removed properties */
	struct	device_node *parent;
	struct	device_node *child;
	struct	device_node *sibling;
	struct	device_node *next;	/* next device of same type */
	struct	device_node *allnext;	/* next in list of all nodes */
	struct	proc_dir_entry *pde;	/* this node's proc directory */
	struct	kref kref;
	unsigned long _flags;
	void	*data;
#if defined(CONFIG_SPARC)
	const char *path_component_name;
	unsigned int unique_id;
	struct of_irq_controller *irq_trans;
#endif
};

#define MAX_PHANDLE_ARGS 8
struct of_phandle_args {
	struct device_node *np;
	int args_count;
	uint32_t args[MAX_PHANDLE_ARGS];
};

#ifdef CONFIG_OF_DYNAMIC
extern struct device_node *of_node_get(struct device_node *node);
extern void of_node_put(struct device_node *node);
#else /* CONFIG_OF_DYNAMIC */
/* Dummy ref counting routines - to be implemented later */
static inline struct device_node *of_node_get(struct device_node *node)
{
	return node;
}
static inline void of_node_put(struct device_node *node) { }
#endif /* !CONFIG_OF_DYNAMIC */

#ifdef CONFIG_OF

/* Pointer for first entry in chain of all nodes. */
extern struct device_node *of_allnodes;
extern struct device_node *of_chosen;
extern struct device_node *of_aliases;
extern raw_spinlock_t devtree_lock;

static inline bool of_have_populated_dt(void)
{
	return of_allnodes != NULL;
}

static inline bool of_node_is_root(const struct device_node *node)
{
	return node && (node->parent == NULL);
}

static inline int of_node_check_flag(struct device_node *n, unsigned long flag)
{
	return test_bit(flag, &n->_flags);
}

static inline void of_node_set_flag(struct device_node *n, unsigned long flag)
{
	set_bit(flag, &n->_flags);
}

static inline void of_node_clear_flag(struct device_node *n, unsigned long flag)
{
	clear_bit(flag, &n->_flags);
}

static inline int of_property_check_flag(struct property *p, unsigned long flag)
{
	return test_bit(flag, &p->_flags);
}

static inline void of_property_set_flag(struct property *p, unsigned long flag)
{
	set_bit(flag, &p->_flags);
}

static inline void of_property_clear_flag(struct property *p, unsigned long flag)
{
	clear_bit(flag, &p->_flags);
}

extern struct device_node *of_find_all_nodes(struct device_node *prev);

/*
 * OF address retrieval & translation
 */

/* Helper to read a big number; size is in cells (not bytes) */
static inline u64 of_read_number(const __be32 *cell, int size)
{
	u64 r = 0;
	while (size--)
		r = (r << 32) | be32_to_cpu(*(cell++));
	return r;
}

/* Like of_read_number, but we want an unsigned long result */
static inline unsigned long of_read_ulong(const __be32 *cell, int size)
{
	/* toss away upper bits if unsigned long is smaller than u64 */
	return of_read_number(cell, size);
}

#include <asm/prom.h>

/* Default #address and #size cells.  Allow arch asm/prom.h to override */
#if !defined(OF_ROOT_NODE_ADDR_CELLS_DEFAULT)
#define OF_ROOT_NODE_ADDR_CELLS_DEFAULT 1
#define OF_ROOT_NODE_SIZE_CELLS_DEFAULT 1
#endif

/* Default string compare functions, Allow arch asm/prom.h to override */
#if !defined(of_compat_cmp)
#define of_compat_cmp(s1, s2, l)	strcasecmp((s1), (s2))
#define of_prop_cmp(s1, s2)		strcmp((s1), (s2))
#define of_node_cmp(s1, s2)		strcasecmp((s1), (s2))
#endif

/* flag descriptions */
#define OF_DYNAMIC	1 /* node and properties were allocated via kmalloc */
#define OF_DETACHED	2 /* node has been detached from the device tree */

#define OF_IS_DYNAMIC(x) test_bit(OF_DYNAMIC, &x->_flags)
#define OF_MARK_DYNAMIC(x) set_bit(OF_DYNAMIC, &x->_flags)

#define OF_BAD_ADDR	((u64)-1)

static inline const char *of_node_full_name(const struct device_node *np)
{
	return np ? np->full_name : "<no-node>";
}

extern struct device_node *of_find_node_by_name(struct device_node *from,
	const char *name);
#define for_each_node_by_name(dn, name) \
	for (dn = of_find_node_by_name(NULL, name); dn; \
	     dn = of_find_node_by_name(dn, name))
extern struct device_node *of_find_node_by_type(struct device_node *from,
	const char *type);
#define for_each_node_by_type(dn, type) \
	for (dn = of_find_node_by_type(NULL, type); dn; \
	     dn = of_find_node_by_type(dn, type))
extern struct device_node *of_find_compatible_node(struct device_node *from,
	const char *type, const char *compat);
#define for_each_compatible_node(dn, type, compatible) \
	for (dn = of_find_compatible_node(NULL, type, compatible); dn; \
	     dn = of_find_compatible_node(dn, type, compatible))
extern struct device_node *of_find_matching_node_and_match(
	struct device_node *from,
	const struct of_device_id *matches,
	const struct of_device_id **match);
static inline struct device_node *of_find_matching_node(
	struct device_node *from,
	const struct of_device_id *matches)
{
	return of_find_matching_node_and_match(from, matches, NULL);
}
#define for_each_matching_node(dn, matches) \
	for (dn = of_find_matching_node(NULL, matches); dn; \
	     dn = of_find_matching_node(dn, matches))
#define for_each_matching_node_and_match(dn, matches, match) \
	for (dn = of_find_matching_node_and_match(NULL, matches, match); \
	     dn; dn = of_find_matching_node_and_match(dn, matches, match))
extern struct device_node *of_find_node_by_path(const char *path);
extern struct device_node *of_find_node_by_phandle(phandle handle);
extern struct device_node *of_get_parent(const struct device_node *node);
extern struct device_node *of_get_next_parent(struct device_node *node);
extern struct device_node *of_get_next_child(const struct device_node *node,
					     struct device_node *prev);
extern struct device_node *of_get_next_available_child(
	const struct device_node *node, struct device_node *prev);

extern struct device_node *of_get_child_by_name(const struct device_node *node,
					const char *name);
#define for_each_child_of_node(parent, child) \
	for (child = of_get_next_child(parent, NULL); child != NULL; \
	     child = of_get_next_child(parent, child))

#define for_each_available_child_of_node(parent, child) \
	for (child = of_get_next_available_child(parent, NULL); child != NULL; \
	     child = of_get_next_available_child(parent, child))

static inline int of_get_child_count(const struct device_node *np)
{
	struct device_node *child;
	int num = 0;

	for_each_child_of_node(np, child)
		num++;

	return num;
}

extern struct device_node *of_find_node_with_property(
	struct device_node *from, const char *prop_name);
#define for_each_node_with_property(dn, prop_name) \
	for (dn = of_find_node_with_property(NULL, prop_name); dn; \
	     dn = of_find_node_with_property(dn, prop_name))

extern struct property *of_find_property(const struct device_node *np,
					 const char *name,
					 int *lenp);
extern int of_property_read_u32_index(const struct device_node *np,
				       const char *propname,
				       u32 index, u32 *out_value);
extern int of_property_read_u8_array(const struct device_node *np,
			const char *propname, u8 *out_values, size_t sz);
extern int of_property_read_u16_array(const struct device_node *np,
			const char *propname, u16 *out_values, size_t sz);
extern int of_property_read_u32_array(const struct device_node *np,
				      const char *propname,
				      u32 *out_values,
				      size_t sz);
extern int of_property_read_u64(const struct device_node *np,
				const char *propname, u64 *out_value);

extern int of_property_read_string(struct device_node *np,
				   const char *propname,
				   const char **out_string);
extern int of_property_read_string_index(struct device_node *np,
					 const char *propname,
					 int index, const char **output);
extern int of_property_match_string(struct device_node *np,
				    const char *propname,
				    const char *string);
extern int of_property_count_strings(struct device_node *np,
				     const char *propname);
extern int of_device_is_compatible(const struct device_node *device,
				   const char *);
extern int of_device_is_available(const struct device_node *device);
extern const void *of_get_property(const struct device_node *node,
				const char *name,
				int *lenp);
#define for_each_property_of_node(dn, pp) \
	for (pp = dn->properties; pp != NULL; pp = pp->next)

extern int of_n_addr_cells(struct device_node *np);
extern int of_n_size_cells(struct device_node *np);
extern const struct of_device_id *of_match_node(
	const struct of_device_id *matches, const struct device_node *node);
extern int of_modalias_node(struct device_node *node, char *modalias, int len);
extern struct device_node *of_parse_phandle(const struct device_node *np,
					    const char *phandle_name,
					    int index);
extern int of_parse_phandle_with_args(const struct device_node *np,
	const char *list_name, const char *cells_name, int index,
	struct of_phandle_args *out_args);
extern int of_count_phandle_with_args(const struct device_node *np,
	const char *list_name, const char *cells_name);

#if defined(CONFIG_OF_DYNAMIC)
extern int of_property_notify(int action, struct device_node *np,
			      struct property *prop);
#else
static inline int of_property_notify(int action, struct device_node *np,
			      struct property *prop)
{
	return 0;
}
#endif

#ifdef CONFIG_PROC_DEVICETREE

extern void of_add_proc_dt_entry(struct device_node *dn);
extern void of_remove_proc_dt_entry(struct device_node *dn);

extern void of_add_proc_dt_prop_entry(struct device_node *np,
		struct property *prop);

extern void of_remove_proc_dt_prop_entry(struct device_node *np,
		struct property *prop);

extern void of_update_proc_dt_prop_entry(struct device_node *np,
		struct property *newprop, struct property *oldprop);
#else

static inline void of_add_proc_dt_entry(struct device_node *dn) { }
static inline void of_remove_proc_dt_entry(struct device_node *dn) { }

static inline void of_add_proc_dt_prop_entry(struct device_node *np,
		struct property *prop) { }

static inline void of_remove_proc_dt_prop_entry(struct device_node *np,
		struct property *prop) { }

static inline void of_update_proc_dt_prop_entry(struct device_node *np,
		struct property *newprop, struct property *oldprop) { }

#endif

extern void of_alias_scan(void * (*dt_alloc)(u64 size, u64 align));
extern int of_alias_get_id(struct device_node *np, const char *stem);

extern int of_machine_is_compatible(const char *compat);

extern int of_add_property(struct device_node *np, struct property *prop);
extern int of_remove_property(struct device_node *np, struct property *prop);
extern int of_update_property(struct device_node *np, struct property *newprop);

/* For updating the device tree at runtime */
#define OF_RECONFIG_ATTACH_NODE		0x0001
#define OF_RECONFIG_DETACH_NODE		0x0002
#define OF_RECONFIG_ADD_PROPERTY	0x0003
#define OF_RECONFIG_REMOVE_PROPERTY	0x0004
#define OF_RECONFIG_UPDATE_PROPERTY	0x0005

struct of_prop_reconfig {
	struct device_node	*dn;
	struct property		*prop;
};

extern int of_reconfig_notifier_register(struct notifier_block *);
extern int of_reconfig_notifier_unregister(struct notifier_block *);
extern int of_reconfig_notify(unsigned long, void *);

extern int of_attach_node(struct device_node *);
extern int of_detach_node(struct device_node *);

#define of_match_ptr(_ptr)	(_ptr)

/*
 * struct property *prop;
 * const __be32 *p;
 * u32 u;
 *
 * of_property_for_each_u32(np, "propname", prop, p, u)
 *         printk("U32 value: %x\n", u);
 */
const __be32 *of_prop_next_u32(struct property *prop, const __be32 *cur,
			       u32 *pu);
#define of_property_for_each_u32(np, propname, prop, p, u)	\
	for (prop = of_find_property(np, propname, NULL),	\
		p = of_prop_next_u32(prop, NULL, &u);		\
		p;						\
		p = of_prop_next_u32(prop, p, &u))

/*
 * struct property *prop;
 * const char *s;
 *
 * of_property_for_each_string(np, "propname", prop, s)
 *         printk("String value: %s\n", s);
 */
const char *of_prop_next_string(struct property *prop, const char *cur);
#define of_property_for_each_string(np, propname, prop, s)	\
	for (prop = of_find_property(np, propname, NULL),	\
		s = of_prop_next_string(prop, NULL);		\
		s;						\
		s = of_prop_next_string(prop, s))

#else /* CONFIG_OF */

static inline const char* of_node_full_name(struct device_node *np)
{
	return "<no-node>";
}

static inline struct device_node *of_find_node_by_name(struct device_node *from,
	const char *name)
{
	return NULL;
}

static inline struct device_node *of_get_parent(const struct device_node *node)
{
	return NULL;
}

static inline bool of_have_populated_dt(void)
{
	return false;
}

#define for_each_child_of_node(parent, child) \
	while (0)

static inline struct device_node *of_get_child_by_name(
					const struct device_node *node,
					const char *name)
{
	return NULL;
}

static inline int of_get_child_count(const struct device_node *np)
{
	return 0;
}

static inline int of_device_is_compatible(const struct device_node *device,
					  const char *name)
{
	return 0;
}

static inline int of_device_is_available(const struct device_node *device)
{
	return 0;
}

static inline struct property *of_find_property(const struct device_node *np,
						const char *name,
						int *lenp)
{
	return NULL;
}

static inline struct device_node *of_find_compatible_node(
						struct device_node *from,
						const char *type,
						const char *compat)
{
	return NULL;
}

static inline int of_property_read_u32_index(const struct device_node *np,
			const char *propname, u32 index, u32 *out_value)
{
	return -ENOSYS;
}

static inline int of_property_read_u8_array(const struct device_node *np,
			const char *propname, u8 *out_values, size_t sz)
{
	return -ENOSYS;
}

static inline int of_property_read_u16_array(const struct device_node *np,
			const char *propname, u16 *out_values, size_t sz)
{
	return -ENOSYS;
}

static inline int of_property_read_u32_array(const struct device_node *np,
					     const char *propname,
					     u32 *out_values, size_t sz)
{
	return -ENOSYS;
}

static inline int of_property_read_string(struct device_node *np,
					  const char *propname,
					  const char **out_string)
{
	return -ENOSYS;
}

static inline int of_property_read_string_index(struct device_node *np,
						const char *propname, int index,
						const char **out_string)
{
	return -ENOSYS;
}

static inline int of_property_count_strings(struct device_node *np,
					    const char *propname)
{
	return -ENOSYS;
}

static inline const void *of_get_property(const struct device_node *node,
				const char *name,
				int *lenp)
{
	return NULL;
}

static inline int of_property_read_u64(const struct device_node *np,
				       const char *propname, u64 *out_value)
{
	return -ENOSYS;
}

static inline int of_property_match_string(struct device_node *np,
					   const char *propname,
					   const char *string)
{
	return -ENOSYS;
}

static inline struct device_node *of_parse_phandle(const struct device_node *np,
						   const char *phandle_name,
						   int index)
{
	return NULL;
}

static inline int of_parse_phandle_with_args(struct device_node *np,
					     const char *list_name,
					     const char *cells_name,
					     int index,
					     struct of_phandle_args *out_args)
{
	return -ENOSYS;
}

static inline int of_count_phandle_with_args(struct device_node *np,
					     const char *list_name,
					     const char *cells_name)
{
	return -ENOSYS;
}

static inline int of_alias_get_id(struct device_node *np, const char *stem)
{
	return -ENOSYS;
}

static inline int of_machine_is_compatible(const char *compat)
{
	return 0;
}

#define of_match_ptr(_ptr)	NULL
#define of_match_node(_matches, _node)	NULL
#define of_property_for_each_u32(np, propname, prop, p, u) \
	while (0)
#define of_property_for_each_string(np, propname, prop, s) \
	while (0)
#endif /* CONFIG_OF */

#ifndef of_node_to_nid
static inline int of_node_to_nid(struct device_node *np)
{
	return numa_node_id();
}

#define of_node_to_nid of_node_to_nid
#endif

/**
 * of_property_read_bool - Findfrom a property
 * @np:		device node from which the property value is to be read.
 * @propname:	name of the property to be searched.
 *
 * Search for a property in a device node.
 * Returns true if the property exist false otherwise.
 */
static inline bool of_property_read_bool(const struct device_node *np,
					 const char *propname)
{
	struct property *prop = of_find_property(np, propname, NULL);

	return prop ? true : false;
}

static inline int of_property_read_u8(const struct device_node *np,
				       const char *propname,
				       u8 *out_value)
{
	return of_property_read_u8_array(np, propname, out_value, 1);
}

static inline int of_property_read_u16(const struct device_node *np,
				       const char *propname,
				       u16 *out_value)
{
	return of_property_read_u16_array(np, propname, out_value, 1);
}

static inline int of_property_read_u32(const struct device_node *np,
				       const char *propname,
				       u32 *out_value)
{
	return of_property_read_u32_array(np, propname, out_value, 1);
}

#if defined(CONFIG_PROC_FS) && defined(CONFIG_PROC_DEVICETREE)
extern void proc_device_tree_add_node(struct device_node *, struct proc_dir_entry *);
extern void proc_device_tree_add_prop(struct proc_dir_entry *pde, struct property *prop);
extern void proc_device_tree_remove_prop(struct proc_dir_entry *pde,
					 struct property *prop);
extern void proc_device_tree_update_prop(struct proc_dir_entry *pde,
					 struct property *newprop,
					 struct property *oldprop);
#endif

/**
 * General utilities for working with live trees.
 *
 * All functions with two leading underscores operate
 * without taking node references, so you either have to
 * own the devtree lock or work on detached trees only.
 */

#ifdef CONFIG_OF

/* iterator for internal use; not references, neither affects devtree lock */
#define __for_each_child_of_node(dn, chld) \
	for (chld = (dn)->child; chld != NULL; chld = chld->sibling)

void __of_free_property(struct property *prop);
void __of_free_tree(struct device_node *node);
struct property *__of_copy_property(const struct property *prop, gfp_t flags);
struct device_node *__of_create_empty_node( const char *name,
		const char *type, const char *full_name,
		phandle phandle, gfp_t flags);
struct device_node *__of_find_node_by_full_name(struct device_node *node,
		const char *full_name);
int of_multi_prop_cmp(const struct property *prop, const char *value);

#else /* !CONFIG_OF */

#define __for_each_child_of_node(dn, chld) \
	while (0)

static inline void __of_free_property(struct property *prop) { }

static inline void __of_free_tree(struct device_node *node) { }

static inline struct property *__of_copy_property(const struct property *prop,
		gfp_t flags)
{
	return NULL;
}

static inline struct device_node *__of_create_empty_node( const char *name,
		const char *type, const char *full_name,
		phandle phandle, gfp_t flags)
{
	return NULL;
}

static inline struct device_node *__of_find_node_by_full_name(struct device_node *node,
		const char *full_name)
{
	return NULL;
}

static inline int of_multi_prop_cmp(const struct property *prop, const char *value)
{
	return -1;
}

#endif	/* !CONFIG_OF */


/* illegal phandle value (set when unresolved) */
#define OF_PHANDLE_ILLEGAL	0xdeadbeef

#ifdef CONFIG_OF_RESOLVE

int of_resolve(struct device_node *resolve);

#else

static inline int of_resolve(struct device_node *resolve)
{
	return -ENOTSUPP;
}

#endif

/**
 * Overlay support
 */

/**
 * struct of_overlay_log_entry	- Holds a DT log entry
 * @node:	list_head for the log list
 * @action:	notifier action
 * @np:		pointer to the device node affected
 * @prop:	pointer to the property affected
 * @old_prop:	hold a pointer to the original property
 *
 * Every modification of the device tree during application of the
 * overlay is held in a list of of_overlay_log_entry structures.
 * That way we can recover from a partial application, or we can
 * revert the overlay properly.
 */
struct of_overlay_log_entry {
	struct list_head node;
	unsigned long action;
	struct device_node *np;
	struct property *prop;
	struct property *old_prop;
};

/**
 * struct of_overlay_device_entry	- Holds an overlay device entry
 * @node:	list_head for the device list
 * @np:		device node pointer to the device node affected
 * @pdev:	pointer to the platform device affected
 * @client:	pointer to the I2C client device affected
 * @state:	new device state
 * @prevstate:	previous device state
 *
 * When the overlay results in a device node's state to change this
 * fact is recorded in a list of device entries. After the overlay
 * is applied we can create/destroy the platform devices according
 * to the new state of the live tree.
 */
struct of_overlay_device_entry {
	struct list_head node;
	struct device_node *np;
	struct platform_device *pdev;
	struct i2c_client *client;
	int prevstate;
	int state;
};

/**
 * struct of_overlay_info	- Holds a single overlay info
 * @target:	target of the overlay operation
 * @overlay:	pointer to the overlay contents node
 * @lock:	Lock to hold when accessing the lists
 * @le_list:	List of the overlay logs
 * @de_list:	List of the overlay records
 * @notifier:	of reconfiguration notifier
 *
 * Holds a single overlay state, including all the overlay logs &
 * records.
 */
struct of_overlay_info {
	struct device_node *target;
	struct device_node *overlay;
	struct mutex lock;
	struct list_head le_list;
	struct list_head de_list;
	struct notifier_block notifier;
	int device_depth;
};

#ifdef CONFIG_OF_OVERLAY

int of_overlay(int count, struct of_overlay_info *ovinfo_tab);
int of_overlay_revert(int count, struct of_overlay_info *ovinfo_tab);

int of_fill_overlay_info(struct device_node *info_node,
		struct of_overlay_info *ovinfo);
int of_build_overlay_info(struct device_node *tree,
		int *cntp, struct of_overlay_info **ovinfop);
int of_free_overlay_info(int cnt, struct of_overlay_info *ovinfo);

#else

static inline int of_overlay(int count, struct of_overlay_info *ovinfo_tab)
{
	return -ENOTSUPP;
}

static inline int of_overlay_revert(int count, struct of_overlay_info *ovinfo_tab)
{
	return -ENOTSUPP;
}

static inline int of_fill_overlay_info(struct device_node *info_node,
		struct of_overlay_info *ovinfo)
{
	return -ENOTSUPP;
}

static inline int of_build_overlay_info(struct device_node *tree,
		int *cntp, struct of_overlay_info **ovinfop)
{
	return -ENOTSUPP;
}

static inline int of_free_overlay_info(int cnt, struct of_overlay_info *ovinfo)
{
	return -ENOTSUPP;
}

#endif

#endif /* _LINUX_OF_H */
