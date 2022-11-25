#include <sys/ioctl.h>
#include <sys/param.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/libio.h>

main ()
{

   unsigned int		search_type, search_key, search_qual;
   isc_entry_type	isc_entry;
   unsigned int		which_table;
#ifdef __hp9000s700
   return_vsc_mod_entry	*vsc_mod_entry;
   union return_mem_entry	*mem_entry;
#endif  /* __hp9000s700 */

   char			command[10];
   int			error,i,j,temp_entry;
   char			got_mem_table,got_mod_table,init_done,done;

   got_mem_table = 0;
   got_mod_table = 0;
   init_done = 0;
   done = 0;

   do
   {
       printf("\n");
       printf("\n");
       printf("I)io_init        G)io_get_table\n");

#ifdef __hp9000s700
       printf("F)io_free_table  S)io_search_isc  E)io_end\n");
#else
       printf("S)io_search_isc  E)io_end\n");
#endif  /* __hp9000s700 */

       printf("Q)uit\n");
       printf("\n");
       printf("Selection> ");

       scanf("%s",command);
       if ((command[0] == 'i') || (command[0] == 'I'))
       {
           error = io_init(O_RDWR);
	   if (error < 0)
	   {
                printf("\n");
		printf("io_init failed with error = %d\n",error);	
	   }
	   else
	       init_done = 1;
       }
       else if ((command[0] == 'e') || (command[0] == 'E'))
       {
           io_end();
	   init_done = 0;
       }
#ifdef __hp9000s700
       else if ((command[0] == 'g') || (command[0] == 'G'))
       {
           printf("\n");
	   printf("Enter which table (0 = Mod table  1 = Memory data)> ");
	   scanf("%d",&which_table);
	   if (which_table == T_IO_MOD_TABLE)
	   {
               error = io_get_table(which_table,&vsc_mod_entry);
	   }
	   else
	   {
               error = io_get_table(which_table,&mem_entry);
	   }
	   if (error < 0)
	   {
               printf("\n");
	       printf("io_get_table failed with error = %d\n",error);	
	   }
	   else
	   {
	       if (which_table == T_IO_MOD_TABLE)
	       {
		   got_mod_table = 1;
                   printf("\n");
		   printf("%d entries in table returned\n",error);
		   for (i = 0; i < error; i++)
		   {
		       printf("iodc_hversion = 0x%x\n",
			       vsc_mod_entry[i].iodc_hversion);
		       printf("iodc_spa = 0x%x\n",
			       vsc_mod_entry[i].iodc_spa);
		       printf("iodc_type = 0x%x\n",
			       vsc_mod_entry[i].iodc_type);
		       printf("iodc_sversion = 0x%x\n",
			       vsc_mod_entry[i].iodc_sversion);
		       printf("iodc_reserved = 0x%x\n",
			       vsc_mod_entry[i].iodc_reserved);
		       printf("iodc_rev = 0x%x\n",
			       vsc_mod_entry[i].iodc_rev);
		       printf("iodc_dep = 0x%x\n",
			       vsc_mod_entry[i].iodc_dep);
		       printf("iodc_check = 0x%x\n",
			       vsc_mod_entry[i].iodc_check);
		       printf("iodc_length = 0x%x\n",
			       vsc_mod_entry[i].iodc_length);
		       printf("hpa = 0x%x\n",
			       vsc_mod_entry[i].hpa);
		       printf("more_pgs= 0x%x\n",
			       vsc_mod_entry[i].more_pgs);
		       if (vsc_mod_entry[i].iodc_type.type==MOD_TYPE_BUS_ADAPTER)
		       {
			   for (j = 0; j < NUM_SBUS_MODS; j++)
			   {
		               printf("ba_mod_table[%d] = %d\n",j,
			          vsc_mod_entry[i].type.ba.ba_mod_table[j]);
			   }
		       }
		       else if (vsc_mod_entry[i].iodc_type.type==MOD_TYPE_FOREIGN_IO)
		       {
			   printf("isc_table.my_isc = 0x%x\n",
				   vsc_mod_entry[i].type.fio.isc_table.my_isc);
			   printf("isc_table.bus_type = 0x%x\n",
				   vsc_mod_entry[i].type.fio.isc_table.bus_type);
			   printf("isc_table.if_id = 0x%x\n",
				   vsc_mod_entry[i].type.fio.isc_table.if_id);
			   printf("isc_table.ftn_no = %d\n",
				   vsc_mod_entry[i].type.fio.isc_table.ftn_no);
			   printf("isc_table.c_major = %d\n",
				   vsc_mod_entry[i].type.fio.isc_table.c_major);
			   printf("isc_table.b_major = %d\n",
				   vsc_mod_entry[i].type.fio.isc_table.b_major);
			   printf("isc_table.hdw_path.num_elements = %d\n",
			      vsc_mod_entry[i].type.fio.isc_table.hdw_path.num_elements);
			   for (j = 0; j < vsc_mod_entry[i].type.fio.isc_table.hdw_path.num_elements; j++)
			   {
			       printf("isc_table.hdw_path[%d] = %d\n",j,
				  vsc_mod_entry[i].type.fio.isc_table.hdw_path.addr[j]);
			   }
			   printf("next_ftn = %d\n",
			      vsc_mod_entry[i].type.fio.next_ftn);
			   printf("ftn_no= %d\n",
			      vsc_mod_entry[i].type.fio.ftn_no);
		       }
		   }
	       }
	       else if (which_table == T_MEM_DATA_TABLE)
	       {
		   got_mod_table = 1;
                   printf("\n");
		   printf("%d entries in table returned\n",error);
		   for (i = 0; i < error; i++)
		   {
		       printf("mem_data.spa1 = 0x%x\n",
			  mem_entry[i].mem_data.spa1);
		       printf("mem_data.spa2 = 0x%x\n",
			  mem_entry[i].mem_data.spa2);
		       printf("mem_data.size = 0x%x\n",
			  mem_entry[i].mem_data.size);
		       printf("mem_data.enabled1 = %d\n",
			  mem_entry[i].mem_data.enabled1);
		       printf("mem_data.enabled2 = %d\n",
			  mem_entry[i].mem_data.enabled2);
		   }
	       }
	   }
       }
       else if ((command[0] == 'f') || (command[0] == 'F'))
       {
           printf("\n");
	   printf("Enter which table (0 = Mod table  1 = Memory data)> ");
	   scanf("%d",&which_table);
	   if (which_table == T_IO_MOD_TABLE)
	   {
               io_free_table(which_table,&vsc_mod_entry);
	   }
	   else
	   {
               io_free_table(which_table,&mem_entry);
	   }
           if (which_table == T_IO_MOD_TABLE)
	       got_mod_table = 0;
	   else
	       got_mem_table = 0;
       }
#endif  /* __hp9000s700 */
       else if ((command[0] == 's') || (command[0] == 'S'))
       {
           printf("\n");
	   printf("Enter search type (0 = first  1 = next  2 = single)> ");
	   scanf("%d",&search_type);
	   printf("Enter search key (0 = all    1 = my_isc  2 = bus_type) \n");

#ifdef __hp9000s700
	   printf("                 (4 = hdw_path)> ");
#endif  /* __hp9000s700 */

	   scanf("%d",&search_key);
	   switch(search_key)
	   {
	       case KEY_ALL:
		   break;

	       case KEY_MY_ISC:
		   printf("Enter isc (in hex)> ");
	           scanf("%x",&(temp_entry));
	           isc_entry.my_isc = (unsigned char)temp_entry;
		   break;

	       case KEY_BUS_TYPE:
		   printf("Enter bus type (in hex)> ");
	           scanf("%x",&(temp_entry));
	           isc_entry.bus_type = (char)temp_entry;
		   break;

#ifdef __hp9000s700
	       case KEY_IF_ID:
		   printf("Enter interface id (in hex)> ");
	           scanf("%x",&(isc_entry.if_id));
		   break;
#endif  /* __hp9000s700 */

	       case KEY_HDW_PATH:
		   printf("Enter number of elements in hdw path> ");
	           scanf("%d",&(isc_entry.hdw_path.num_elements));
		   for (i = 0; i < isc_entry.hdw_path.num_elements; i++)
		   {
		       printf("Enter element number %d> ",i);
	               scanf("%d",&(isc_entry.hdw_path.addr[i]));
		   }
		   break;

	   }
#ifdef __hp9000s700
	   printf("Enter search qual (0 = none  1 = init  2 = function no)\n");
	   printf("                  (3 = init & ftn)> ");
	   scanf("%d",&search_qual);
	   if (search_qual & QUAL_FTN_NO)
	   {
	       printf("Enter function number %d> ");
               scanf("%d",&(isc_entry.ftn_no));
	   }
#else
	   search_qual = QUAL_NONE;
#endif  /* __hp9000s700 */

           error = io_search_isc(search_type, search_key, search_qual,
				 &isc_entry);
	   if (error < 0)
	   {
		printf("io_search_isc failed with error = %d\n",error);	
	   }
	   else
	   {
               printf("\n");
	       printf("isc_entry.my_isc = 0x%x\n",
		       isc_entry.my_isc);
	       printf("isc_entry.bus_type = 0x%x\n",
		       isc_entry.bus_type);
	       printf("isc_entry.if_id = 0x%x\n",
		       isc_entry.if_id);
	       printf("isc_entry.ftn_no = %d\n",
		       isc_entry.ftn_no);
	       printf("isc_entry.c_major = %d\n",
		       isc_entry.c_major);
	       printf("isc_entry.b_major = %d\n",
		       isc_entry.b_major);
	       printf("isc_entry.hdw_path.num_elements = %d\n",
	          isc_entry.hdw_path.num_elements);
	       for (j = 0; j < isc_entry.hdw_path.num_elements; j++)
	       {
	           printf("isc_entry.hdw_path[%d] = %d\n",j,
		      isc_entry.hdw_path.addr[j]);
	       }
	   }
       }
       else if ((command[0] == 'q') || (command[0] == 'Q'))
       {
	   if (init_done)
	   {
               printf("\n");
	       printf("Need to do io_end\n");
	   }
	   else if (got_mem_table)
	   {
               printf("\n");
	       printf("Need to do io_free_table for mem_table\n");
	   }
	   else if (got_mod_table)
	   {
               printf("\n");
	       printf("Need to do io_free_table for mod_table\n");
	   }
	   else
	       done = 1;
       }
       else
       {
           printf("\n");
           printf("INVALID SELECTION\n");	
       }

   } while (done == 0);

}
