/**
 * @file   status.h
 * @author Aliaksiej Artamonaŭ <aliaksiej.artamonau@gmail.com>
 * @date   Sun Sep 12 20:11:36 2010
 *
 * @brief  Functions allowing to add entries to a status directory under
 * module directory.
 *
 *
 */


#ifndef _STATUS_H_
#define _STATUS_H_


#include <linux/sysfs.h>
#include <linux/stat.h>
#include <linux/stringify.h>


struct status_attr_t;


/// Typedef for functions showing attributes' values.
typedef ssize_t
(*status_attr_show_t)(const char *, void *, char *buffer);


/// Structure representing an attribute in status directory.
struct status_attr_t {
  struct attribute   attr;   /**< Holds attribute name and mode. */
  status_attr_show_t show;   /**< Function to be called when one reads
                              *   attribute's file. */

  bool               has_file;  /**< Indicates whether file has been
                                 * created for this attribute or not. */
  struct list_head   list;      /**< Gathers all attributes in status
                                 * directory in one list. */

  void              *data;      /**< Private data. */
};


/**
 * Initializer for status_attr_t structures.
 *
 * @param _name attribute name
 * @param _show function to be called when one reads attribute's file
 * @param _data private data
 */
#define STATUS_ATTR(_name, _show, _data)                      \
  { .attr     = { .name = __stringify(_name),                 \
                  .mode = S_IRUGO,                            \
                },                                            \
    .show     = _show,                                        \
    .has_file = false,                                        \
    .data     = _data,                                        \
  }


/**
 * Declares static structure representing status attribute. Name of the
 * declared structure is of the for 'status_attr_##_name'.
 *
 * @param _name attribute name
 * @param _show function to be called when one reads attribute's file
 * @param _data private data
 */
#define STATUS_ATTR_DECLARE(_name, _show, _data)              \
  static struct status_attr_t status_attr_##_name =           \
    STATUS_ATTR(_name, _show, _data)


/**
 * Initializes status attribute.
 *
 * @param status_attr attribute to initialize
 * @param name        name of the attribute; must not be freed while attribute
 *                    is used
 * @param show        function to be called when one reads attribute's file
 * @param data        private data
 */
static inline void
status_attr_init(struct status_attr_t *status_attr,
                 const char *name,
                 status_attr_show_t show,
                 void *data)
{
  status_attr->attr.name = name;
  status_attr->attr.mode = S_IRUGO;

  status_attr->show      = show;
  status_attr->has_file  = false;
  status_attr->data      = data;
}


/**
 * Creates a status subdirectory in module's directory.
 *
 *
 * @retval  0 status directory created successfully
 * @retval <0 error occurred
 */
int
status_create(void);


/**
 * Removes status subdirectory from module's directory. All attributes must be
 * removed by now.
 *
 */
void
status_remove(void);


/**
 * Creates a file corresponding to attribute in status directory.
 *
 * @param status_attr attribute to be used
 *
 * @retval  0 file created successfully
 * @retval <0 error occurred
 */
int
status_create_file(struct status_attr_t *status_attr);


/**
 * Creates files corresponding to attributes. In case of errors all previously
 * created files are removed.
 *
 * @param status_attrs attributes
 * @param count        number of elements in 'status_attrs' array
 *
 * @retval  0 files created successfully
 * @retval <0 error occurred
 */
int
status_create_files(struct status_attr_t status_attrs[], size_t count);


/**
 * Removes file entry corresponding to the attribute from status directory.
 *
 * @param status_attr attribute
 */
void
status_remove_file(struct status_attr_t *status_attr);


/**
 * Removes file entries corresponding to the attributes.
 *
 * @param status_attrs attributes
 * @param count       number of elements in 'status_attrs' array
 */
void
status_remove_files(struct status_attr_t status_attrs[], size_t count);


/**
 * Removes all attributes from status directory.
 *
 */
void
status_remove_all_files(void);


#endif /* _STATUS_H_ */
