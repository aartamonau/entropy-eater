/**
 * @file   eater_status.h
 * @author Aliaksiej Artamonaŭ <aliaksiej.artamonau@gmail.com>
 * @date   Sun Sep 12 20:11:36 2010
 *
 * @brief  Functions allowing to add entries to a status directory under
 * module directory.
 *
 *
 */


#ifndef _EATER_STATUS_H_
#define _EATER_STATUS_H_


#include <linux/sysfs.h>
#include <linux/stat.h>
#include <linux/stringify.h>


/// Structure representing an attribute in status directory.
struct eater_status_attribute_t {
  struct attribute attr;                  /**< Holds attribute name and mode. */
  ssize_t (*show)(const char *, char *);  /**< Function to be called when one
                                           * reads attribute's file. */

  bool             has_file;              /**< Indicates whether file has been
                                           * created for this attribute or not.
                                           * */
  struct list_head list;                  /**< Gathers all attributes in status
                                           * directory in one list. */
};


/// Typedef for functions showing attributes' values.
typedef ssize_t (*eater_attribute_show_t)(const char *name, char *buffer);


/**
 * Initializer for eater_status_attribute_t structures.
 *
 * @param _name attribute name
 * @param _show function to be called when one reads attribute's file
 */
#define EATER_STATUS_ATTR(_name, _show)                       \
  { .attr     = { .name = __stringify(_name),                 \
                  .mode = S_IRUGO,                            \
                },                                            \
    .show     = _show,                                        \
    .has_file = false,                                        \
  }


/**
 * Declares static structure representing status attribute. Name of the
 * declared structure is of the for 'eater_attr_##_name'.
 *
 * @param _name attribute name
 * @param _show function to be called when one reads attribute's file
 */
#define EATER_STATUS_ATTR_DECLARE(_name, _show)               \
  static struct eater_status_attribute_t eater_attr_##_name = \
    EATER_STATUS_ATTR(_name, _show)


/**
 * Creates a status subdirectory in module's directory.
 *
 *
 * @retval  0 status directory created successfully
 * @retval <0 error occurred
 */
int
eater_status_create(void);


/**
 * Removes status subdirectory from module's directory. All attributes must be
 * removed by now.
 *
 */
void
eater_status_remove(void);


/**
 * Creates a file corresponding to attribute in status directory.
 *
 * @param attr attribute to be used
 *
 * @retval  0 file created successfully
 * @retval <0 error occurred
 */
int
eater_status_create_file(struct eater_status_attribute_t *eater_attr);


/**
 * Creates files corresponding to attributes. In case of errors all previously
 * created files are removed.
 *
 * @param eater_attrs attributes
 * @param count       number of elements in 'eater_attrs' array
 *
 * @retval  0 files created successfully
 * @retval <0 error occurred
 */
int
eater_status_create_files(struct eater_status_attribute_t eater_attrs[],
                          size_t count);


/**
 * Removes file entry corresponding to the attribute from status directory.
 *
 * @param eater_attr attribute
 */
void
eater_status_remove_file(struct eater_status_attribute_t *eater_attr);


/**
 * Removes file entries corresponding to the attributes.
 *
 * @param eater_attrs attributes
 * @param count       number of elements in 'eater_attrs' array
 */
void
eater_status_remove_files(struct eater_status_attribute_t eater_attrs[],
                          size_t count);


/**
 * Removes all attributes from status directory.
 *
 */
void
eater_status_remove_all(void);


#endif /* _EATER_STATUS_H_ */
