struct	req_struct
{
	int	magic;
	int	request;
	int	pid;
	int	status;
	char	data[ FIOC_BUFFER_SIZE + 1 ];
};
#define FCT_IOCTL_MAGIC 0x4643
