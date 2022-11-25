/* $Revision: 70.4 $ */

/*
 * (c) Copyright 1989,1990,1991,1992 Hewlett-Packard Company, all rights reserved.
 *
 * #   #  ##  ##### ####
 * ##  # #  #   #   #
 * # # # #  #   #   ####
 * #  ## #  #   #   #
 * #   #  ##    #   ####
 *
 * Please read the README file with this source code. It contains important
 * information on conventions which must be followed to make sure that this
 * code keeps working on all the platforms and in all the environments in
 * which it is used.
 */

/* This value is limited to the range of positive values
 * represented by a character. */
#define MAXPARAMS 127
#define MAX_MACRO_NAME 256
#define TABLESIZE 499

#define NON_PARAM -1

#define NORMAL_PARAM 1
#define QUOTED_PARAM 2
#define SQUEEZED_PARAM 3
#define CURRENT_LINE 4
#define CURRENT_FILE 5
#define CURRENT_DATE 6
#define CURRENT_TIME 7

#define is_special_char(ch) ((ch) >= NORMAL_PARAM && (ch) <= CURRENT_TIME)

typedef struct define_node {
	char *name;
	char *string;
	int  has_params;
	int  num_params;
	char **param_array;
	boolean disabled;
	struct define_node *next;
} t_define;

typedef struct name_node {
	char *name;
	struct name_node *next;
} t_name_list;

typedef char *t_param_array[MAXPARAMS];

extern t_define *get_define();
