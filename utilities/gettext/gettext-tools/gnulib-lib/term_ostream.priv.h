/* DO NOT EDIT! GENERATED AUTOMATICALLY! */

/* Field layout of superclass.  */
#include "ostream.priv.h"

/* Field layout of term_ostream class.  */
struct term_ostream_representation
{
  struct ostream_representation base;
   
  int fd;
  char *filename;
   
                                 
  int max_colors;                
  int no_color_video;            
  char *set_a_foreground;        
  char *set_foreground;          
  char *set_a_background;        
  char *set_background;          
  char *orig_pair;               
  char *enter_bold_mode;         
  char *enter_italics_mode;      
  char *exit_italics_mode;       
  char *enter_underline_mode;    
  char *exit_underline_mode;     
  char *exit_attribute_mode;     
   
  bool supports_foreground;
  bool supports_background;
  colormodel_t colormodel;
  bool supports_weight;
  bool supports_posture;
  bool supports_underline;
   
  char *buffer;                  
  attributes_t *attrbuffer;      
  size_t buflen;                 
  size_t allocated;              
  attributes_t curr_attr;        
  attributes_t simp_attr;        
};
