#ifndef forms_h
#define forms_h

/*
 * creates a new jabber:x:data form
 * returns the node created added
 */
xmlnode form_new(xmlnode parent,const char *title,const char *instructions);

/*
 * adds a field to a jabber:x:data form
 * returns the field added
 */
xmlnode form_add_field(xmlnode form,const char *type,const char *var,
				const char *label,const char *val,int required);

/*
 * adds an option to list field of jabber:x:data form
 * returns the node added
 */
xmlnode form_add_option(xmlnode field,const char *label,const char *val);

/*
 * adds "fixed" field to a jabber:x:data form
 * returns the field added
 */
xmlnode form_add_fixed(xmlnode form,const char *val);

/*
 * creates a new jabber:x:data result form
 * returns the node created added
 */
xmlnode form_new_result(const char *title);

/*
 * adds a field declaration to a jabber:x:data report
 * returns the field added
 */
xmlnode form_add_result_field(xmlnode form,const char *var,const char *label,const char *type);

/*
 * adds an item jabber:x:data report
 * returns the item added
 */
xmlnode form_add_result_item(xmlnode form);


/*
 * adds a value to a jabber:x:data report item
 * returns the field added
 */
xmlnode form_add_result_value(xmlnode item,const char *var,const char *val);

#endif
