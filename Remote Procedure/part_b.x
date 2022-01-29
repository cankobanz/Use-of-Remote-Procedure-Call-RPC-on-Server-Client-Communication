struct inputs{
	int a;
	int b;
	char blackboxPath[200];
};

program PART_B_PROG{
	version PART_B_VERS{
		string part_b(inputs)=1;
	}=1;
}=0x19845673;