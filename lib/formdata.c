/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2020, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at https://carl.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/

#include "carl_setup.h"

#include <carl/carl.h>

#include "formdata.h"
#if !defined(CARL_DISABLE_HTTP) && !defined(CARL_DISABLE_MIME)

#if defined(HAVE_LIBGEN_H) && defined(HAVE_BASENAME)
#include <libgen.h>
#endif

#include "urldata.h" /* for struct Curl_easy */
#include "mime.h"
#include "non-ascii.h"
#include "vtls/vtls.h"
#include "strcase.h"
#include "sendf.h"
#include "strdup.h"
#include "rand.h"
#include "warnless.h"
/* The last 3 #include files should be in this order */
#include "carl_printf.h"
#include "carl_memory.h"
#include "memdebug.h"


#define HTTPPOST_PTRNAME CARL_HTTPPOST_PTRNAME
#define HTTPPOST_FILENAME CARL_HTTPPOST_FILENAME
#define HTTPPOST_PTRCONTENTS CARL_HTTPPOST_PTRCONTENTS
#define HTTPPOST_READFILE CARL_HTTPPOST_READFILE
#define HTTPPOST_PTRBUFFER CARL_HTTPPOST_PTRBUFFER
#define HTTPPOST_CALLBACK CARL_HTTPPOST_CALLBACK
#define HTTPPOST_BUFFER CARL_HTTPPOST_BUFFER

/***************************************************************************
 *
 * AddHttpPost()
 *
 * Adds a HttpPost structure to the list, if parent_post is given becomes
 * a subpost of parent_post instead of a direct list element.
 *
 * Returns newly allocated HttpPost on success and NULL if malloc failed.
 *
 ***************************************************************************/
static struct carl_httppost *
AddHttpPost(char *name, size_t namelength,
            char *value, carl_off_t contentslength,
            char *buffer, size_t bufferlength,
            char *contenttype,
            long flags,
            struct carl_slist *contentHeader,
            char *showfilename, char *userp,
            struct carl_httppost *parent_post,
            struct carl_httppost **httppost,
            struct carl_httppost **last_post)
{
  struct carl_httppost *post;
  post = calloc(1, sizeof(struct carl_httppost));
  if(post) {
    post->name = name;
    post->namelength = (long)(name?(namelength?namelength:strlen(name)):0);
    post->contents = value;
    post->contentlen = contentslength;
    post->buffer = buffer;
    post->bufferlength = (long)bufferlength;
    post->contenttype = contenttype;
    post->contentheader = contentHeader;
    post->showfilename = showfilename;
    post->userp = userp;
    post->flags = flags | CARL_HTTPPOST_LARGE;
  }
  else
    return NULL;

  if(parent_post) {
    /* now, point our 'more' to the original 'more' */
    post->more = parent_post->more;

    /* then move the original 'more' to point to ourselves */
    parent_post->more = post;
  }
  else {
    /* make the previous point to this */
    if(*last_post)
      (*last_post)->next = post;
    else
      (*httppost) = post;

    (*last_post) = post;
  }
  return post;
}

/***************************************************************************
 *
 * AddFormInfo()
 *
 * Adds a FormInfo structure to the list presented by parent_form_info.
 *
 * Returns newly allocated FormInfo on success and NULL if malloc failed/
 * parent_form_info is NULL.
 *
 ***************************************************************************/
static struct FormInfo *AddFormInfo(char *value,
                                    char *contenttype,
                                    struct FormInfo *parent_form_info)
{
  struct FormInfo *form_info;
  form_info = calloc(1, sizeof(struct FormInfo));
  if(form_info) {
    if(value)
      form_info->value = value;
    if(contenttype)
      form_info->contenttype = contenttype;
    form_info->flags = HTTPPOST_FILENAME;
  }
  else
    return NULL;

  if(parent_form_info) {
    /* now, point our 'more' to the original 'more' */
    form_info->more = parent_form_info->more;

    /* then move the original 'more' to point to ourselves */
    parent_form_info->more = form_info;
  }

  return form_info;
}

/***************************************************************************
 *
 * FormAdd()
 *
 * Stores a formpost parameter and builds the appropriate linked list.
 *
 * Has two principal functionalities: using files and byte arrays as
 * post parts. Byte arrays are either copied or just the pointer is stored
 * (as the user requests) while for files only the filename and not the
 * content is stored.
 *
 * While you may have only one byte array for each name, multiple filenames
 * are allowed (and because of this feature CARLFORM_END is needed after
 * using CARLFORM_FILE).
 *
 * Examples:
 *
 * Simple name/value pair with copied contents:
 * carl_formadd (&post, &last, CARLFORM_COPYNAME, "name",
 * CARLFORM_COPYCONTENTS, "value", CARLFORM_END);
 *
 * name/value pair where only the content pointer is remembered:
 * carl_formadd (&post, &last, CARLFORM_COPYNAME, "name",
 * CARLFORM_PTRCONTENTS, ptr, CARLFORM_CONTENTSLENGTH, 10, CARLFORM_END);
 * (if CARLFORM_CONTENTSLENGTH is missing strlen () is used)
 *
 * storing a filename (CONTENTTYPE is optional!):
 * carl_formadd (&post, &last, CARLFORM_COPYNAME, "name",
 * CARLFORM_FILE, "filename1", CARLFORM_CONTENTTYPE, "plain/text",
 * CARLFORM_END);
 *
 * storing multiple filenames:
 * carl_formadd (&post, &last, CARLFORM_COPYNAME, "name",
 * CARLFORM_FILE, "filename1", CARLFORM_FILE, "filename2", CARLFORM_END);
 *
 * Returns:
 * CARL_FORMADD_OK             on success
 * CARL_FORMADD_MEMORY         if the FormInfo allocation fails
 * CARL_FORMADD_OPTION_TWICE   if one option is given twice for one Form
 * CARL_FORMADD_NULL           if a null pointer was given for a char
 * CARL_FORMADD_MEMORY         if the allocation of a FormInfo struct failed
 * CARL_FORMADD_UNKNOWN_OPTION if an unknown option was used
 * CARL_FORMADD_INCOMPLETE     if the some FormInfo is not complete (or error)
 * CARL_FORMADD_MEMORY         if a HttpPost struct cannot be allocated
 * CARL_FORMADD_MEMORY         if some allocation for string copying failed.
 * CARL_FORMADD_ILLEGAL_ARRAY  if an illegal option is used in an array
 *
 ***************************************************************************/

static
CARLFORMcode FormAdd(struct carl_httppost **httppost,
                     struct carl_httppost **last_post,
                     va_list params)
{
  struct FormInfo *first_form, *current_form, *form = NULL;
  CARLFORMcode return_value = CARL_FORMADD_OK;
  const char *prevtype = NULL;
  struct carl_httppost *post = NULL;
  CARLformoption option;
  struct carl_forms *forms = NULL;
  char *array_value = NULL; /* value read from an array */

  /* This is a state variable, that if TRUE means that we're parsing an
     array that we got passed to us. If FALSE we're parsing the input
     va_list arguments. */
  bool array_state = FALSE;

  /*
   * We need to allocate the first struct to fill in.
   */
  first_form = calloc(1, sizeof(struct FormInfo));
  if(!first_form)
    return CARL_FORMADD_MEMORY;

  current_form = first_form;

  /*
   * Loop through all the options set. Break if we have an error to report.
   */
  while(return_value == CARL_FORMADD_OK) {

    /* first see if we have more parts of the array param */
    if(array_state && forms) {
      /* get the upcoming option from the given array */
      option = forms->option;
      array_value = (char *)forms->value;

      forms++; /* advance this to next entry */
      if(CARLFORM_END == option) {
        /* end of array state */
        array_state = FALSE;
        continue;
      }
    }
    else {
      /* This is not array-state, get next option */
      option = va_arg(params, CARLformoption);
      if(CARLFORM_END == option)
        break;
    }

    switch(option) {
    case CARLFORM_ARRAY:
      if(array_state)
        /* we don't support an array from within an array */
        return_value = CARL_FORMADD_ILLEGAL_ARRAY;
      else {
        forms = va_arg(params, struct carl_forms *);
        if(forms)
          array_state = TRUE;
        else
          return_value = CARL_FORMADD_NULL;
      }
      break;

      /*
       * Set the Name property.
       */
    case CARLFORM_PTRNAME:
#ifdef CARL_DOES_CONVERSIONS
      /* Treat CARLFORM_PTR like CARLFORM_COPYNAME so that libcarl will copy
       * the data in all cases so that we'll have safe memory for the eventual
       * conversion.
       */
#else
      current_form->flags |= HTTPPOST_PTRNAME; /* fall through */
#endif
      /* FALLTHROUGH */
    case CARLFORM_COPYNAME:
      if(current_form->name)
        return_value = CARL_FORMADD_OPTION_TWICE;
      else {
        char *name = array_state?
          array_value:va_arg(params, char *);
        if(name)
          current_form->name = name; /* store for the moment */
        else
          return_value = CARL_FORMADD_NULL;
      }
      break;
    case CARLFORM_NAMELENGTH:
      if(current_form->namelength)
        return_value = CARL_FORMADD_OPTION_TWICE;
      else
        current_form->namelength =
          array_state?(size_t)array_value:(size_t)va_arg(params, long);
      break;

      /*
       * Set the contents property.
       */
    case CARLFORM_PTRCONTENTS:
      current_form->flags |= HTTPPOST_PTRCONTENTS;
      /* FALLTHROUGH */
    case CARLFORM_COPYCONTENTS:
      if(current_form->value)
        return_value = CARL_FORMADD_OPTION_TWICE;
      else {
        char *value =
          array_state?array_value:va_arg(params, char *);
        if(value)
          current_form->value = value; /* store for the moment */
        else
          return_value = CARL_FORMADD_NULL;
      }
      break;
    case CARLFORM_CONTENTSLENGTH:
      current_form->contentslength =
        array_state?(size_t)array_value:(size_t)va_arg(params, long);
      break;

    case CARLFORM_CONTENTLEN:
      current_form->flags |= CARL_HTTPPOST_LARGE;
      current_form->contentslength =
        array_state?(carl_off_t)(size_t)array_value:va_arg(params, carl_off_t);
      break;

      /* Get contents from a given file name */
    case CARLFORM_FILECONTENT:
      if(current_form->flags & (HTTPPOST_PTRCONTENTS|HTTPPOST_READFILE))
        return_value = CARL_FORMADD_OPTION_TWICE;
      else {
        const char *filename = array_state?
          array_value:va_arg(params, char *);
        if(filename) {
          current_form->value = strdup(filename);
          if(!current_form->value)
            return_value = CARL_FORMADD_MEMORY;
          else {
            current_form->flags |= HTTPPOST_READFILE;
            current_form->value_alloc = TRUE;
          }
        }
        else
          return_value = CARL_FORMADD_NULL;
      }
      break;

      /* We upload a file */
    case CARLFORM_FILE:
      {
        const char *filename = array_state?array_value:
          va_arg(params, char *);

        if(current_form->value) {
          if(current_form->flags & HTTPPOST_FILENAME) {
            if(filename) {
              char *fname = strdup(filename);
              if(!fname)
                return_value = CARL_FORMADD_MEMORY;
              else {
                form = AddFormInfo(fname, NULL, current_form);
                if(!form) {
                  free(fname);
                  return_value = CARL_FORMADD_MEMORY;
                }
                else {
                  form->value_alloc = TRUE;
                  current_form = form;
                  form = NULL;
                }
              }
            }
            else
              return_value = CARL_FORMADD_NULL;
          }
          else
            return_value = CARL_FORMADD_OPTION_TWICE;
        }
        else {
          if(filename) {
            current_form->value = strdup(filename);
            if(!current_form->value)
              return_value = CARL_FORMADD_MEMORY;
            else {
              current_form->flags |= HTTPPOST_FILENAME;
              current_form->value_alloc = TRUE;
            }
          }
          else
            return_value = CARL_FORMADD_NULL;
        }
        break;
      }

    case CARLFORM_BUFFERPTR:
      current_form->flags |= HTTPPOST_PTRBUFFER|HTTPPOST_BUFFER;
      if(current_form->buffer)
        return_value = CARL_FORMADD_OPTION_TWICE;
      else {
        char *buffer =
          array_state?array_value:va_arg(params, char *);
        if(buffer) {
          current_form->buffer = buffer; /* store for the moment */
          current_form->value = buffer; /* make it non-NULL to be accepted
                                           as fine */
        }
        else
          return_value = CARL_FORMADD_NULL;
      }
      break;

    case CARLFORM_BUFFERLENGTH:
      if(current_form->bufferlength)
        return_value = CARL_FORMADD_OPTION_TWICE;
      else
        current_form->bufferlength =
          array_state?(size_t)array_value:(size_t)va_arg(params, long);
      break;

    case CARLFORM_STREAM:
      current_form->flags |= HTTPPOST_CALLBACK;
      if(current_form->userp)
        return_value = CARL_FORMADD_OPTION_TWICE;
      else {
        char *userp =
          array_state?array_value:va_arg(params, char *);
        if(userp) {
          current_form->userp = userp;
          current_form->value = userp; /* this isn't strictly true but we
                                          derive a value from this later on
                                          and we need this non-NULL to be
                                          accepted as a fine form part */
        }
        else
          return_value = CARL_FORMADD_NULL;
      }
      break;

    case CARLFORM_CONTENTTYPE:
      {
        const char *contenttype =
          array_state?array_value:va_arg(params, char *);
        if(current_form->contenttype) {
          if(current_form->flags & HTTPPOST_FILENAME) {
            if(contenttype) {
              char *type = strdup(contenttype);
              if(!type)
                return_value = CARL_FORMADD_MEMORY;
              else {
                form = AddFormInfo(NULL, type, current_form);
                if(!form) {
                  free(type);
                  return_value = CARL_FORMADD_MEMORY;
                }
                else {
                  form->contenttype_alloc = TRUE;
                  current_form = form;
                  form = NULL;
                }
              }
            }
            else
              return_value = CARL_FORMADD_NULL;
          }
          else
            return_value = CARL_FORMADD_OPTION_TWICE;
        }
        else {
          if(contenttype) {
            current_form->contenttype = strdup(contenttype);
            if(!current_form->contenttype)
              return_value = CARL_FORMADD_MEMORY;
            else
              current_form->contenttype_alloc = TRUE;
          }
          else
            return_value = CARL_FORMADD_NULL;
        }
        break;
      }
    case CARLFORM_CONTENTHEADER:
      {
        /* this "cast increases required alignment of target type" but
           we consider it OK anyway */
        struct carl_slist *list = array_state?
          (struct carl_slist *)(void *)array_value:
          va_arg(params, struct carl_slist *);

        if(current_form->contentheader)
          return_value = CARL_FORMADD_OPTION_TWICE;
        else
          current_form->contentheader = list;

        break;
      }
    case CARLFORM_FILENAME:
    case CARLFORM_BUFFER:
      {
        const char *filename = array_state?array_value:
          va_arg(params, char *);
        if(current_form->showfilename)
          return_value = CARL_FORMADD_OPTION_TWICE;
        else {
          current_form->showfilename = strdup(filename);
          if(!current_form->showfilename)
            return_value = CARL_FORMADD_MEMORY;
          else
            current_form->showfilename_alloc = TRUE;
        }
        break;
      }
    default:
      return_value = CARL_FORMADD_UNKNOWN_OPTION;
      break;
    }
  }

  if(CARL_FORMADD_OK != return_value) {
    /* On error, free allocated fields for all nodes of the FormInfo linked
       list without deallocating nodes. List nodes are deallocated later on */
    struct FormInfo *ptr;
    for(ptr = first_form; ptr != NULL; ptr = ptr->more) {
      if(ptr->name_alloc) {
        Curl_safefree(ptr->name);
        ptr->name_alloc = FALSE;
      }
      if(ptr->value_alloc) {
        Curl_safefree(ptr->value);
        ptr->value_alloc = FALSE;
      }
      if(ptr->contenttype_alloc) {
        Curl_safefree(ptr->contenttype);
        ptr->contenttype_alloc = FALSE;
      }
      if(ptr->showfilename_alloc) {
        Curl_safefree(ptr->showfilename);
        ptr->showfilename_alloc = FALSE;
      }
    }
  }

  if(CARL_FORMADD_OK == return_value) {
    /* go through the list, check for completeness and if everything is
     * alright add the HttpPost item otherwise set return_value accordingly */

    post = NULL;
    for(form = first_form;
        form != NULL;
        form = form->more) {
      if(((!form->name || !form->value) && !post) ||
         ( (form->contentslength) &&
           (form->flags & HTTPPOST_FILENAME) ) ||
         ( (form->flags & HTTPPOST_FILENAME) &&
           (form->flags & HTTPPOST_PTRCONTENTS) ) ||

         ( (!form->buffer) &&
           (form->flags & HTTPPOST_BUFFER) &&
           (form->flags & HTTPPOST_PTRBUFFER) ) ||

         ( (form->flags & HTTPPOST_READFILE) &&
           (form->flags & HTTPPOST_PTRCONTENTS) )
        ) {
        return_value = CARL_FORMADD_INCOMPLETE;
        break;
      }
      if(((form->flags & HTTPPOST_FILENAME) ||
          (form->flags & HTTPPOST_BUFFER)) &&
         !form->contenttype) {
        char *f = (form->flags & HTTPPOST_BUFFER)?
          form->showfilename : form->value;
        char const *type;
        type = Curl_mime_contenttype(f);
        if(!type)
          type = prevtype;
        if(!type)
          type = FILE_CONTENTTYPE_DEFAULT;

        /* our contenttype is missing */
        form->contenttype = strdup(type);
        if(!form->contenttype) {
          return_value = CARL_FORMADD_MEMORY;
          break;
        }
        form->contenttype_alloc = TRUE;
      }
      if(form->name && form->namelength) {
        /* Name should not contain nul bytes. */
        size_t i;
        for(i = 0; i < form->namelength; i++)
          if(!form->name[i]) {
            return_value = CARL_FORMADD_NULL;
            break;
          }
        if(return_value != CARL_FORMADD_OK)
          break;
      }
      if(!(form->flags & HTTPPOST_PTRNAME) &&
         (form == first_form) ) {
        /* Note that there's small risk that form->name is NULL here if the
           app passed in a bad combo, so we better check for that first. */
        if(form->name) {
          /* copy name (without strdup; possibly not null-terminated) */
          form->name = Curl_memdup(form->name, form->namelength?
                                   form->namelength:
                                   strlen(form->name) + 1);
        }
        if(!form->name) {
          return_value = CARL_FORMADD_MEMORY;
          break;
        }
        form->name_alloc = TRUE;
      }
      if(!(form->flags & (HTTPPOST_FILENAME | HTTPPOST_READFILE |
                          HTTPPOST_PTRCONTENTS | HTTPPOST_PTRBUFFER |
                          HTTPPOST_CALLBACK)) && form->value) {
        /* copy value (without strdup; possibly contains null characters) */
        size_t clen  = (size_t) form->contentslength;
        if(!clen)
          clen = strlen(form->value) + 1;

        form->value = Curl_memdup(form->value, clen);

        if(!form->value) {
          return_value = CARL_FORMADD_MEMORY;
          break;
        }
        form->value_alloc = TRUE;
      }
      post = AddHttpPost(form->name, form->namelength,
                         form->value, form->contentslength,
                         form->buffer, form->bufferlength,
                         form->contenttype, form->flags,
                         form->contentheader, form->showfilename,
                         form->userp,
                         post, httppost,
                         last_post);

      if(!post) {
        return_value = CARL_FORMADD_MEMORY;
        break;
      }

      if(form->contenttype)
        prevtype = form->contenttype;
    }
    if(CARL_FORMADD_OK != return_value) {
      /* On error, free allocated fields for nodes of the FormInfo linked
         list which are not already owned by the httppost linked list
         without deallocating nodes. List nodes are deallocated later on */
      struct FormInfo *ptr;
      for(ptr = form; ptr != NULL; ptr = ptr->more) {
        if(ptr->name_alloc) {
          Curl_safefree(ptr->name);
          ptr->name_alloc = FALSE;
        }
        if(ptr->value_alloc) {
          Curl_safefree(ptr->value);
          ptr->value_alloc = FALSE;
        }
        if(ptr->contenttype_alloc) {
          Curl_safefree(ptr->contenttype);
          ptr->contenttype_alloc = FALSE;
        }
        if(ptr->showfilename_alloc) {
          Curl_safefree(ptr->showfilename);
          ptr->showfilename_alloc = FALSE;
        }
      }
    }
  }

  /* Always deallocate FormInfo linked list nodes without touching node
     fields given that these have either been deallocated or are owned
     now by the httppost linked list */
  while(first_form) {
    struct FormInfo *ptr = first_form->more;
    free(first_form);
    first_form = ptr;
  }

  return return_value;
}

/*
 * carl_formadd() is a public API to add a section to the multipart formpost.
 *
 * @unittest: 1308
 */

CARLFORMcode carl_formadd(struct carl_httppost **httppost,
                          struct carl_httppost **last_post,
                          ...)
{
  va_list arg;
  CARLFORMcode result;
  va_start(arg, last_post);
  result = FormAdd(httppost, last_post, arg);
  va_end(arg);
  return result;
}

/*
 * carl_formget()
 * Serialize a carl_httppost struct.
 * Returns 0 on success.
 *
 * @unittest: 1308
 */
int carl_formget(struct carl_httppost *form, void *arg,
                 carl_formget_callback append)
{
  CARLcode result;
  carl_mimepart toppart;

  Curl_mime_initpart(&toppart, NULL); /* default form is empty */
  result = Curl_getformdata(NULL, &toppart, form, NULL);
  if(!result)
    result = Curl_mime_prepare_headers(&toppart, "multipart/form-data",
                                       NULL, MIMESTRATEGY_FORM);

  while(!result) {
    char buffer[8192];
    size_t nread = Curl_mime_read(buffer, 1, sizeof(buffer), &toppart);

    if(!nread)
      break;

    if(nread > sizeof(buffer) || append(arg, buffer, nread) != nread) {
      result = CARLE_READ_ERROR;
      if(nread == CARL_READFUNC_ABORT)
        result = CARLE_ABORTED_BY_CALLBACK;
    }
  }

  Curl_mime_cleanpart(&toppart);
  return (int) result;
}

/*
 * carl_formfree() is an external function to free up a whole form post
 * chain
 */
void carl_formfree(struct carl_httppost *form)
{
  struct carl_httppost *next;

  if(!form)
    /* no form to free, just get out of this */
    return;

  do {
    next = form->next;  /* the following form line */

    /* recurse to sub-contents */
    carl_formfree(form->more);

    if(!(form->flags & HTTPPOST_PTRNAME))
      free(form->name); /* free the name */
    if(!(form->flags &
         (HTTPPOST_PTRCONTENTS|HTTPPOST_BUFFER|HTTPPOST_CALLBACK))
      )
      free(form->contents); /* free the contents */
    free(form->contenttype); /* free the content type */
    free(form->showfilename); /* free the faked file name */
    free(form);       /* free the struct */
    form = next;
  } while(form); /* continue */
}


/* Set mime part name, taking care of non null-terminated name string. */
static CARLcode setname(carl_mimepart *part, const char *name, size_t len)
{
  char *zname;
  CARLcode res;

  if(!name || !len)
    return carl_mime_name(part, name);
  zname = malloc(len + 1);
  if(!zname)
    return CARLE_OUT_OF_MEMORY;
  memcpy(zname, name, len);
  zname[len] = '\0';
  res = carl_mime_name(part, zname);
  free(zname);
  return res;
}

/*
 * Curl_getformdata() converts a linked list of "meta data" into a mime
 * structure. The input list is in 'post', while the output is stored in
 * mime part at '*finalform'.
 *
 * This function will not do a failf() for the potential memory failures but
 * should for all other errors it spots. Just note that this function MAY get
 * a NULL pointer in the 'data' argument.
 */

CARLcode Curl_getformdata(struct Curl_easy *data,
                          carl_mimepart *finalform,
                          struct carl_httppost *post,
                          carl_read_callback fread_func)
{
  CARLcode result = CARLE_OK;
  carl_mime *form = NULL;
  carl_mimepart *part;
  struct carl_httppost *file;

  Curl_mime_cleanpart(finalform); /* default form is empty */

  if(!post)
    return result; /* no input => no output! */

  form = carl_mime_init(data);
  if(!form)
    result = CARLE_OUT_OF_MEMORY;

  if(!result)
    result = carl_mime_subparts(finalform, form);

  /* Process each top part. */
  for(; !result && post; post = post->next) {
    /* If we have more than a file here, create a mime subpart and fill it. */
    carl_mime *multipart = form;
    if(post->more) {
      part = carl_mime_addpart(form);
      if(!part)
        result = CARLE_OUT_OF_MEMORY;
      if(!result)
        result = setname(part, post->name, post->namelength);
      if(!result) {
        multipart = carl_mime_init(data);
        if(!multipart)
          result = CARLE_OUT_OF_MEMORY;
      }
      if(!result)
        result = carl_mime_subparts(part, multipart);
    }

    /* Generate all the part contents. */
    for(file = post; !result && file; file = file->more) {
      /* Create the part. */
      part = carl_mime_addpart(multipart);
      if(!part)
        result = CARLE_OUT_OF_MEMORY;

      /* Set the headers. */
      if(!result)
        result = carl_mime_headers(part, file->contentheader, 0);

      /* Set the content type. */
      if(!result && file->contenttype)
        result = carl_mime_type(part, file->contenttype);

      /* Set field name. */
      if(!result && !post->more)
        result = setname(part, post->name, post->namelength);

      /* Process contents. */
      if(!result) {
        carl_off_t clen = post->contentslength;

        if(post->flags & CARL_HTTPPOST_LARGE)
          clen = post->contentlen;
        if(!clen)
          clen = -1;

        if(post->flags & (HTTPPOST_FILENAME | HTTPPOST_READFILE)) {
          if(!strcmp(file->contents, "-")) {
            /* There are a few cases where the code below won't work; in
               particular, freopen(stdin) by the caller is not guaranteed
               to result as expected. This feature has been kept for backward
               compatibility: use of "-" pseudo file name should be avoided. */
            result = carl_mime_data_cb(part, (carl_off_t) -1,
                                       (carl_read_callback) fread,
                                       CARLX_FUNCTION_CAST(carl_seek_callback,
                                                           fseek),
                                       NULL, (void *) stdin);
          }
          else
            result = carl_mime_filedata(part, file->contents);
          if(!result && (post->flags & HTTPPOST_READFILE))
            result = carl_mime_filename(part, NULL);
        }
        else if(post->flags & HTTPPOST_BUFFER)
          result = carl_mime_data(part, post->buffer,
                                  post->bufferlength? post->bufferlength: -1);
        else if(post->flags & HTTPPOST_CALLBACK)
          /* the contents should be read with the callback and the size is set
             with the contentslength */
          result = carl_mime_data_cb(part, clen,
                                     fread_func, NULL, NULL, post->userp);
        else {
          result = carl_mime_data(part, post->contents, (ssize_t) clen);
#ifdef CARL_DOES_CONVERSIONS
          /* Convert textual contents now. */
          if(!result && data && part->datasize)
            result = Curl_convert_to_network(data, part->data, part->datasize);
#endif
        }
      }

      /* Set fake file name. */
      if(!result && post->showfilename)
        if(post->more || (post->flags & (HTTPPOST_FILENAME | HTTPPOST_BUFFER |
                                        HTTPPOST_CALLBACK)))
          result = carl_mime_filename(part, post->showfilename);
    }
  }

  if(result)
    Curl_mime_cleanpart(finalform);

  return result;
}

#else
/* if disabled */
CARLFORMcode carl_formadd(struct carl_httppost **httppost,
                          struct carl_httppost **last_post,
                          ...)
{
  (void)httppost;
  (void)last_post;
  return CARL_FORMADD_DISABLED;
}

int carl_formget(struct carl_httppost *form, void *arg,
                 carl_formget_callback append)
{
  (void) form;
  (void) arg;
  (void) append;
  return CARL_FORMADD_DISABLED;
}

void carl_formfree(struct carl_httppost *form)
{
  (void)form;
  /* does nothing HTTP is disabled */
}

#endif  /* if disabled */
