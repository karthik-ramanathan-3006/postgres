/**
 * @brief - An enum that represents all the C data types
 * encountered while scanning the Postgres code-base.
 * 
 */
typedef enum c_type {
    T_BOOL = 0,
    T_SHORT,
    T_INT,
    T_LONG,
    T_FLOAT,
    T_DOUBLE,
    T_ENUM,
    T_PTR,
    T_UNKNOWN,
} c_type;

/**
 * @brief - A struct that represents an X or a feature.
 * An X contains a name and a C data type.
 * 
 */
typedef struct field {
    c_type type;
    char *name;
} field;

/**
 * @brief - Represents a TScout Operating Unit
 * 
 */
typedef struct OperatingUnit {
    int  ou_index;
    char *name;
    int  num_xs;
    field *fields;
} ou;

// Features go here
field feat_none[0];

ou ou_list[] = {
    (ou){ OU_INDEX, OU_NAME, NUM_Xs, OU_Xs },
};