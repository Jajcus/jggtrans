/* $Id: forms.c,v 1.6 2003/04/16 11:10:17 jajcus Exp $ */

/*
 *  (C) Copyright 2002-2005 Jacek Konieczny [jajcus(a)jajcus,net]
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License Version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "ggtrans.h"

/*
 * creates a new jabber:x:data form
 * returns the node created added
 */
xmlnode form_new(xmlnode parent,const char *title,const char *instructions){
xmlnode form,tag;

	if (parent)
		form=xmlnode_insert_tag(parent,"x");
	else
		form=xmlnode_new_tag("x");

	xmlnode_put_attrib(form,"xmlns","jabber:x:data");
	xmlnode_put_attrib(form,"type","form");
	tag=xmlnode_insert_tag(form,"title");
	xmlnode_insert_cdata(tag,title,-1);
	tag=xmlnode_insert_tag(form,"instructions");
	xmlnode_insert_cdata(tag,instructions,-1);
	return form;
}

/*
 * creates a new jabber:x:data result form
 * returns the node created added
 */
xmlnode form_new_result(const char *title){
xmlnode form,tag;

	form=xmlnode_new_tag("x");
	xmlnode_put_attrib(form,"xmlns","jabber:x:data");
	xmlnode_put_attrib(form,"type","result");
	tag=xmlnode_insert_tag(form,"title");
	xmlnode_insert_cdata(tag,title,-1);
	return form;
}


/*
 * adds a field to a jabber:x:data form
 * returns the field added
 */
xmlnode form_add_field(xmlnode form,const char *type,const char *var,
				const char *label,const char *val,int required){
xmlnode field,value;

	field=xmlnode_insert_tag(form,"field");
	xmlnode_put_attrib(field,"type",type);
	xmlnode_put_attrib(field,"var",var);
	if (required) xmlnode_insert_tag(field,"required");
	xmlnode_put_attrib(field,"label",label);
	if (val){
		value=xmlnode_insert_tag(field,"value");
		xmlnode_insert_cdata(value,val,-1);
	}
	return field;
}

/*
 * adds an option to list field of jabber:x:data form
 * returns the node added
 */
xmlnode form_add_option(xmlnode field,const char *label,const char *val){
xmlnode option,value;

	option=xmlnode_insert_tag(field,"option");
	xmlnode_put_attrib(option,"label",label);
	value=xmlnode_insert_tag(option,"value");
	xmlnode_insert_cdata(value,val,-1);
	return option;
}


/*
 * adds "fixed" field to a jabber:x:data form
 * returns the field added
 */
xmlnode form_add_fixed(xmlnode form,const char *val){
xmlnode field,value;

	field=xmlnode_insert_tag(form,"field");
	xmlnode_put_attrib(field,"type","fixed");
	value=xmlnode_insert_tag(field,"value");
	xmlnode_insert_cdata(value,val,-1);
	return field;
}

/*
 * adds a field declaration to a jabber:x:data report
 * returns the field added
 */
xmlnode form_add_result_field(xmlnode form,const char *var,const char *label,const char *type){
xmlnode rep,field;

	rep=xmlnode_get_tag(form,"reported");
	if (rep==NULL){
		rep=xmlnode_insert_tag(form,"reported");
	}
	field=xmlnode_insert_tag(rep,"field");
	xmlnode_put_attrib(field,"var",var);
	xmlnode_put_attrib(field,"label",label);
	if (type!=NULL)
		xmlnode_put_attrib(field,"type",type);
	return field;
}

/*
 * adds an item jabber:x:data report
 * returns the item added
 */
xmlnode form_add_result_item(xmlnode form){
xmlnode item;

	item=xmlnode_insert_tag(form,"item");
	return item;
}

/*
 * adds a value to a jabber:x:data report item
 * returns the field added
 */
xmlnode form_add_result_value(xmlnode item,const char *var,const char *val){
xmlnode field,value;

	field=xmlnode_insert_tag(item,"field");
	xmlnode_put_attrib(field,"var",var);
	value=xmlnode_insert_tag(field,"value");
	if (val!=NULL)
		xmlnode_insert_cdata(value,val,-1);
	return field;
}


