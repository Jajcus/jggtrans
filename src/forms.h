#ifndef forms_h
#define forms_h

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


#endif
