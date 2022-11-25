
#define         sto_c_max_h_coding_entries      (24*32)
#define         sto_c_current_version_number    1
#define         sto_c_max_login_entries         2000
#define         sto_c_max_device_name_length    30


typedef struct  sto_t_status {
                 unsigned       port_defined : 1;
                 unsigned       board_defined : 1;
        };

typedef struct  sto_t_entry {
        unsigned        short   int     link_index;
        struct          sto_t_status    status;
        unsigned        int             IP_address;
        unsigned        int             port_number;
        unsigned        int             board_number;
        char                            device_name[sto_c_max_device_name_length+1];
        };



typedef struct  sto_t_login_file {
        unsigned        int             version_number;
        unsigned        int             length;
        unsigned        short   int     max_depth;
        unsigned        short   int     reserved_1;
        unsigned        short   int     reserved_2;
        unsigned        short   int     reserved_3;
        unsigned        short   int     reserved_4;
        unsigned        short   int     h_coding_first_index[sto_c_max_h_coding_entries];
        struct          sto_t_entry     entry_ptr[sto_c_max_login_entries+1];
        };

