#include <linux/module.h>

#include "utils/trace.h"
#include "utils/assert.h"

#include "status/status.h"


/**
 * Casts a pointer to embedded attribute to containing status attribute.
 *
 * @param attr pointer to attribute
 *
 * @return containing eater status attribute
 */
#define TO_STATUS_ATTR(attr) \
  container_of(attr, struct status_attr_t, attr)


/// A structure that holds context of status directory.
struct status_context_t {
  struct kobject   object;      /**< Associated kernel object. */
  struct list_head attrs;       /**< List of all attributes in directory. */
};


/**
 * Casts a pointer to kernel object to the #status_context_t structure
 * in which it's embedded.
 *
 * @param object pointer to kobject
 *
 * @return containing #status_context_t structure
 */
#define TO_STATUS(object) \
  container_of(object, struct status_context_t, object)


/**
 * Function that is called when 'attr' is read. Dispatches the work to the
 * attribute-specific 'show' function specified in #status_attr_t.
 *
 * @param object containing kobject; must be context.object here;
 * @param attr   attribute that is read
 * @param buffer buffer to store value of attribute
 *
 * @retval >=0 number of bytes written to buffer
 * @retval  <0 error code
 */
static ssize_t
status_sysfs_show(struct kobject *object, struct attribute *attr, char *buffer);


/// Sysfs operations for status directory.
static struct sysfs_ops status_sysfs_ops = {
  .show  = status_sysfs_show,
};


/// Kernel type for #status_context_t.
static struct kobj_type status_ktype = {
  .sysfs_ops = &status_sysfs_ops,
};


/// Global context.
static struct status_context_t context;


int
status_create(void)
{
  int ret;

  ret = kobject_init_and_add(&context.object, &status_ktype,
                             &THIS_MODULE->mkobj.kobj, "status");
  if (0 != ret) {
    TRACE_ERR("Failed to initialize status directory kobject. Error: %d.", ret);
    kobject_put(&context.object);
  }

  INIT_LIST_HEAD(&context.attrs);

  return ret;
}


void
status_remove(void)
{
  ASSERT( list_empty(&context.attrs) );

  kobject_put(&context.object);
}


int
status_create_file(struct status_attr_t *status_attr)
{
  int ret;

  ASSERT( !status_attr->has_file );

  ret = sysfs_create_file(&context.object, &status_attr->attr);
  if (ret != 0) {
    TRACE_ERR("Failed to create sysfs entry for %s attribute. Error: %d.",
              status_attr->attr.name, ret);
    return ret;
  }

  status_attr->has_file = true;
  list_add(&status_attr->list, &context.attrs);

  return 0;
}


void
status_remove_file(struct status_attr_t *status_attr)
{
  ASSERT( status_attr->has_file );

  sysfs_remove_file(&context.object, &status_attr->attr);

  status_attr->has_file = false;
  list_del(&status_attr->list);
}


static ssize_t
status_sysfs_show(struct kobject *object, struct attribute *attr, char *buffer)
{
  struct status_attr_t *status_attr = TO_STATUS_ATTR(attr);

  ASSERT( TO_STATUS(object) == &context );

  return status_attr->show(status_attr->attr.name, status_attr->data, buffer);
}


void
status_remove_all_files(void)
{
  struct status_attr_t *status_attr;
  struct status_attr_t *tmp;

  list_for_each_entry_safe(status_attr, tmp, &context.attrs, list) {
    status_remove_file(status_attr);
  }
}


void
status_remove_files(struct status_attr_t status_attrs[], size_t count)
{
  int i;

  for (i = 0; i < count; ++i) {
    status_remove_file(&status_attrs[i]);
  }
}


int
status_create_files(struct status_attr_t status_attrs[], size_t count)
{
  int i;
  int j;
  int ret;

  for (i = 0; i < count; ++i) {
    ret = status_create_file(&status_attrs[i]);
    if (ret != 0) {
      for (j = i - 1; j >= 0; --j) {
        status_remove_file(&status_attrs[j]);
      }

      return ret;
    }
  }

  return 0;
}
