#include <linux/module.h>

#include "utils/trace.h"
#include "utils/assert.h"

#include "eater_status.h"


/**
 * Casts a pointer to embedded attribute to containing eater status attribute.
 *
 * @param attr pointer to attribute
 *
 * @return containing eater status attribute
 */
#define TO_EATER_ATTR(attr) \
  container_of(attr, struct eater_status_attribute_t, attr)


/// A structure that holds context of status directory.
struct eater_status_context_t {
  struct kobject   object;      /**< Associated kernel object. */
  struct list_head attrs;       /**< List of all attributes in directory. */
};


/**
 * Casts a pointer to kernel object to the #eater_status_context_t structure
 * in which it's embedded.
 *
 * @param object pointer to kobject
 *
 * @return containing #eater_status_context_t structure
 */
#define TO_EATER(object) \
  container_of(object, struct eater_status_context_t, object)


/**
 * Function that is called when 'attr' is read. Dispatches the work to the
 * attribute-specific 'show' function specified in #eater_status_attribute_t.
 *
 * @param object containing kobject; must be context.object here;
 * @param attr   attribute that is read
 * @param buffer buffer to store value of attribute
 *
 * @retval >=0 number of bytes written to buffer
 * @retval  <0 error code
 */
static ssize_t
eater_status_sysfs_show(struct kobject *object, struct attribute *attr,
                        char *buffer);


/// Sysfs operations for status directory.
static struct sysfs_ops eater_status_sysfs_ops = {
  .show  = eater_status_sysfs_show,
};


/// Kernel type for #eater_status_context_t.
static struct kobj_type eater_status_ktype = {
  .sysfs_ops = &eater_status_sysfs_ops,
};


/// Global context.
static struct eater_status_context_t context;


int
eater_status_create(void)
{
  int ret;

  ret = kobject_init_and_add(&context.object, &eater_status_ktype,
                             &THIS_MODULE->mkobj.kobj, "status");
  if (0 != ret) {
    TRACE_ERR("Failed to initialize status directory kobject. Error: %d.", ret);
    kobject_put(&context.object);
  }

  INIT_LIST_HEAD(&context.attrs);

  return ret;
}


void
eater_status_remove(void)
{
  ASSERT( list_empty(&context.attrs) );

  kobject_put(&context.object);
}


int
eater_status_create_file(struct eater_status_attribute_t *eater_attr)
{
  int ret;

  ASSERT( !eater_attr->has_file );

  ret = sysfs_create_file(&context.object, &eater_attr->attr);
  if (ret != 0) {
    TRACE_ERR("Failed to create sysfs entry for %s attribute. Error: %d.",
              eater_attr->attr.name, ret);
    return ret;
  }

  eater_attr->has_file = true;
  list_add(&eater_attr->list, &context.attrs);

  return 0;
}


void
eater_status_remove_file(struct eater_status_attribute_t *eater_attr)
{
  ASSERT( eater_attr->has_file );

  sysfs_remove_file(&context.object, &eater_attr->attr);

  eater_attr->has_file = false;
  list_del(&eater_attr->list);
}


static ssize_t
eater_status_sysfs_show(struct kobject *object, struct attribute *attr,
                        char *buffer)
{
  struct eater_status_attribute_t *eater_attr = TO_EATER_ATTR(attr);

  ASSERT( TO_EATER(object) == &context );

  return eater_attr->show(eater_attr->attr.name, buffer);
}


void
eater_status_remove_all(void)
{
  struct eater_status_attribute_t *eater_attr;
  struct eater_status_attribute_t *tmp;

  list_for_each_entry_safe(eater_attr, tmp, &context.attrs, list) {
    eater_status_remove_file(eater_attr);
  }
}
