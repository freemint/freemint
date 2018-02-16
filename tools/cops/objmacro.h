/*
 * COPS (c) 1995 - 2003 Sven & Wilfried Behne
 *                 2004 F.Naumann & O.Skancke
 *
 * A XCONTROL compatible CPX manager.
 *
 * This file is part of COPS.
 *
 * COPS is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * COPS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with XaAES; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _objmacro_h
#define _objmacro_h

/*----------------------------------------------------------------------------------------*/
/* Objektmanipulation                                                                     */
/*----------------------------------------------------------------------------------------*/
#define	obj_NORMAL(tree, obj)			(tree[obj].ob_state &= 0xff00)

#define	obj_SELECTED(tree, obj)			(tree[obj].ob_state |= OS_SELECTED)
#define	obj_DESELECTED(tree, obj)		(tree[obj].ob_state &= ~OS_SELECTED)

#define	obj_DISABLED(tree, obj)			(tree[obj].ob_state |= OS_DISABLED)
#define	obj_ENABLED(tree, obj)			(tree[obj].ob_state &= ~OS_DISABLED)

#define	obj_SELECTABLE(tree, obj)		(tree[obj].ob_flags |= OF_SELECTABLE)
#define	obj_NOT_SELECTABLE(tree, obj)		(tree[obj].ob_flags &= ~OF_SELECTABLE)

#define	obj_DEFAULT(tree, obj)			(tree[obj].ob_flags |= OF_DEFAULT)
#define	obj_NOT_DEFAULT(tree, obj)		(tree[obj].ob_flags &= ~OF_DEFAULT)

#define	obj_EXIT(tree, obj)			(tree[obj].ob_flags |= OF_EXIT)
#define	obj_NOT_EXIT(tree, obj)			(tree[obj].ob_flags &= ~OF_EXIT)

#define	obj_EDITABLE(tree, obj)			(tree[obj].ob_flags |= OF_EDITABLE)
#define	obj_NOT_EDITABLE(tree, obj)		(tree[obj].ob_flags &= ~OF_EDITABLE)

#define	obj_TOUCHEXIT(tree, obj)		(tree[obj].ob_flags |= OF_TOUCHEXIT)
#define	obj_NOT_TOUCHEXIT(tree, obj)		(tree[obj].ob_flags &= ~OF_TOUCHEXIT)

#define	obj_HIDDEN(tree, obj)			(tree[obj].ob_flags |= OF_HIDETREE)
#define	obj_VISIBLE(tree, obj)			(tree[obj].ob_flags &= ~OF_HIDETREE)

/*----------------------------------------------------------------------------------------*/
/* Objektstatus                                                                           */
/*----------------------------------------------------------------------------------------*/
#define	is_obj_NORMAL(tree, obj)		 (tree[obj].ob_state & 0xff)

#define	is_obj_SELECTED(tree, obj)		 (tree[obj].ob_state & OS_SELECTED)
#define	is_obj_DESELECTED(tree, obj)		!(tree[obj].ob_state & OS_SELECTED)

#define	is_obj_DISABLED(tree, obj)		 (tree[obj].ob_state & OS_DISABLED)
#define	is_obj_ENABLED(tree, obj)		!(tree[obj].ob_state & OS_DISABLED)

#define	is_obj_SELECTABLE(tree, obj)		 (tree[obj].ob_flags & OF_SELECTABLE)
#define	is_obj_NOT_SELECTABLE(tree, obj)	!(tree[obj].ob_flags & OF_SELECTABLE)

#define	is_obj_DEFAULT(tree, obj)		 (tree[obj].ob_flags & OF_DEFAULT)
#define	is_obj_NOT_DEFAULT(tree, obj)		!(tree[obj].ob_flags & OF_DEFAULT)

#define	is_obj_EXIT(tree, obj)			 (tree[obj].ob_flags & OF_EXIT)
#define	is_obj_NOT_EXIT(tree, obj)		!(tree[obj].ob_flags & OF_EXIT)

#define	is_obj_EDITABLE(tree, obj)		 (tree[obj].ob_flags & OF_EDITABLE)
#define	is_obj_NOT_EDITABLE(tree, obj)		!(tree[obj].ob_flags & OF_EDITABLE)

#define	is_obj_TOUCHEXIT(tree, obj)		 (tree[obj].ob_flags & OF_TOUCHEXIT)
#define	is_obj_NOT_TOUCHEXIT(tree, obj)		!(tree[obj].ob_flags & OF_TOUCHEXIT)

#define	is_obj_HIDDEN(tree, obj)		 (tree[obj].ob_flags & OF_HIDETREE)
#define	is_obj_VISIBLE(tree, obj)		!(tree[obj].ob_flags & OF_HIDETREE)

#endif /* _objmacro_h */
