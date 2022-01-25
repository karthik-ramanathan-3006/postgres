typedef struct OperatingUnit {
    int  ou_index;
    char *name;
    int  num_xs;
    char *features[1];
} ou;

ou ou_list[] = {
    (ou){ OU_INDEX, OU_NAME, NUM_Xs, {OU_Xs} },    
};