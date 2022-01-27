import re
import sys
from typing import List, Tuple

from clang.cindex import TypeKind
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
OU_EXCLUDED_FEATURES = [
    "query_id",
    "left_child_plan_node_id",
    "right_child_plan_node_id",
    "statement_timestamp"
]

def aggregate_features(ou: model.OperatingUnit):
    """
    For the given OU
    """
    features_list = []
    for feature in ou.features_list:
        for variable in feature.bpf_tuple:
            if variable.name in OU_EXCLUDED_FEATURES:
                continue
            features_list.append((variable.name, variable.c_type))

    return features_list

def add_features(features_string: str, feat_index: int, ou_xs: List[Tuple]):
    """
    For the given OU, this function extracts the features, appends to the
    given string and returns it.
    """
    features_string += "\n"
    features_struct_list = []
    for x in ou_xs:
        (name, value) = x
        type_kind = "T_UNKNOWN"
        if value == TypeKind.POINTER:
            type_kind = "T_PTR"
        elif value == TypeKind.INT or value == TypeKind.UINT:
            type_kind = "T_INT"
        elif value == TypeKind.LONG or value == TypeKind.ULONG:
            type_kind = "T_LONG"
        elif value == TypeKind.SHORT:
            type_kind = "T_SHORT"
        elif value == TypeKind.DOUBLE:
            type_kind = "T_DOUBLE"
        elif value == TypeKind.ENUM:
            type_kind = "T_ENUM"
        elif value == TypeKind.BOOL:
            type_kind = "T_BOOL"
        else:
            type_kind = str(value)
        features_struct_list.append("{ %s, \"%s\" }" % (type_kind, name))

    features_struct = str.join(", ", features_struct_list)
    features_string += "field feat_%d[] = { " % (feat_index) + features_struct + " };"

    return features_string

def fill_in_template(ou_string: str, ou_index: int, node_type: int, ou_xs: List[Tuple]):
    """
    For the given OU, this function fills in the autogeneration template.
    """
    ou_string = ou_string.replace("OU_INDEX", "%d" % (ou_index))        # Replace the index
    ou_string = ou_string.replace("OU_NAME", '\"%s\"' % (node_type))    # Replace the name of the OU
    ou_string = ou_string.replace("NUM_Xs", "%d" % (len(ou_xs)))        # Comput and replace number of Xs

    if ou_xs:
        ou_string = ou_string.replace("OU_Xs", "feat_%d" % (ou_index))  # Finally, add the list of Xs
    else:
        ou_string = ou_string.replace("OU_Xs", "feat_none")

    return ou_string


if __name__ == "__main__":
    """
    Main function to generate the TScout Xs.
    """
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
                    "features": aggregate_features(ou)
                }

    # Open and analyse the codegen file.
    with open(str(CODEGEN_TEMPLATE_PATH), "r") as template:
        text = template.read()

        matches = re.findall(r"\(ou\)\{.*\}\,", text)                      # Find a seq. that matches "(ou){.*},"
        assert(len(matches) == 1)

        feat_matcher = re.findall(r"\/\/ Features go here", text)
        assert(len(matches) == 1)

        match = matches[0]
        feat_match = feat_matcher[0]

        ou_struct_list = []
        features_list_string = ""
        # For each OU, generate the features and fill in the autogeneration template.
        for (key, value) in OU_TO_FEATURE_LIST_MAP.items():
            ou_string = match[:] # Copy the matching string by value
            if value:
                ou_xs =  value["features"]
                ou_string = fill_in_template(ou_string, key, value["ou_name"], ou_xs)
                features_list_string = add_features(features_list_string, key, ou_xs)

            else:
                # Print defaults
                ou_string = fill_in_template(ou_string, -1, "", [])

            ou_struct_list.append(ou_string)

        ou_struct_list_string = str.join("\n", ou_struct_list)

        text = text.replace(match, ou_struct_list_string)
        text = text.replace(feat_match, features_list_string)
        with open(CODEGEN_FILE_PATH, "w") as gen_file:
            gen_file.write(text)
