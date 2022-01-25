import re
import sys

from pathlib import Path

# We're assuming that this script is housed in postgres/cmudb/extensions/tscout
# We calculate the path of TScout relative to this extension and add it to the PythonPath temporarily.
TSCOUT_EXTENSION_PATH = Path(__file__).parent
CODEGEN_TEMPLATE_PATH = Path.joinpath(TSCOUT_EXTENSION_PATH, "operating_unit_codegen.c")
CODEGEN_FILE_PATH = Path.joinpath(TSCOUT_EXTENSION_PATH, "operating_unit_features.h")

TSCOUT_PATH = Path.joinpath(TSCOUT_EXTENSION_PATH, Path("../../tscout/"))
sys.path.append(TSCOUT_PATH.resolve().__str__())

# And now, we can import TScout
from tscout import model

OU_TO_FEATURE_LIST_MAP = {}
OU_EXCEPTIONS = {
    "ExecHashJoinImpl": "T_HashJoin"
}

def aggregate_feature(ou):
    features_list = []
    for feature in ou.features_list:
        for variable in feature.bpf_tuple:
            features_list.append(variable.name)

    return features_list

def fill_in_template(ou_string, ou_index, node_type, ou_xs):
    ou_string = ou_string.replace("OU_INDEX", "%d" % (ou_index))        # Replace the index
    ou_string = ou_string.replace("OU_NAME", '\"%s\"' % (node_type))    # Replace the name of the OU
    ou_string = ou_string.replace("NUM_Xs", "%d" % (len(ou_xs)))        # Comput and replace number of Xs
    
    Xs = " \"" + str.join(", ", ou_xs) + "\" "
    ou_string = ou_string.replace("OU_Xs", Xs)                          # Finally, add the list of Xs

    return ou_string

if __name__ == "__main__":
    modeler = model.Model()

    # Fetch the NodeTag enum.
    pg_mapping = modeler.get_enum_value_map('NodeTag')
    for i in range(len(pg_mapping)):
        OU_TO_FEATURE_LIST_MAP[i] = {}
    
    # Populate the NodeTag details 
    for (index, ou) in enumerate(modeler.operating_units):
        if ou.name().startswith("Exec"):
            struct_name = ou.name()[len("Exec"): ]
            pg_struct_name = "T_" + struct_name
            pg_enum_index = None

            if pg_struct_name in pg_mapping.keys():
                pg_enum_index = pg_mapping[pg_struct_name]
            
            elif ou.name() in OU_EXCEPTIONS:
                pg_struct_name = OU_EXCEPTIONS[ou.name()]
                pg_enum_index = pg_mapping[pg_struct_name]

            if pg_enum_index:
                OU_TO_FEATURE_LIST_MAP[pg_enum_index] = {
                    "ou_index": index,
                    "pg_enum_index": pg_mapping[pg_struct_name],
                    "ou_name":  ou.name(),
                    "features": aggregate_feature(ou)
                }

    with open(str(CODEGEN_TEMPLATE_PATH), "r") as template:
        text = template.read()

        matches = re.findall(r"\(ou\)\{.*\}\,", text)                      # Find a seq. that matches "(ou){.*},"
        assert(len(matches) == 1)

        match = matches[0]

        ou_struct_list = []
        for (key, value) in OU_TO_FEATURE_LIST_MAP.items():
            ou_string = match[:] # Copy the matching string by value
            if value:
                ou_xs =  value["features"]
                ou_string = fill_in_template(ou_string, key, value["ou_name"], ou_xs)

            else:
                # Print defaults
                ou_string = fill_in_template(ou_string, -1, "", [])

            ou_struct_list.append(ou_string)
        
        ou_struct_list_string = str.join("\n", ou_struct_list)
        
        text = text.replace(match, ou_struct_list_string)
        with open(CODEGEN_FILE_PATH, "w") as gen_file:
            gen_file.write(text)
