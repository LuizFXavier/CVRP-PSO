import re

def ler_instancia(file_name, get_coords = False):
    file = None

    try:
        file = open(file_name)
    except FileNotFoundError:
        exit("Arquivo não encontrado")
    
    dados = {"NAME":"",
             "COORDS":[],
             "DIMENSION":0,
             "CAPACITY":0,
             "COMMENT": 0}
    
    line = file.readline().rstrip()
    number_regex = "[0-9]*\\.?[0-9]+"

    while line != "NODE_COORD_SECTION":
        matches = re.findall("[\\w-]+", line)
        
        if matches[0] == "NAME":
            dados["NAME"] = matches[1]
        elif matches[0] == "DIMENSION":
            dados["DIMENSION"] = int(re.findall(number_regex, line)[0])
        elif matches[0] == "CAPACITY":
            dados["CAPACITY"] = int(re.findall(number_regex, line)[0])
        elif matches[0] == "COMMENT":
            value_str = re.findall("value: "+number_regex, line)
            if value_str:
                dados["COMMENT"] = float(re.findall(number_regex, value_str[0])[0])
            else:
                dados["COMMENT"] = float(re.findall(number_regex, line)[0])
        
        line = file.readline().rstrip()

    if get_coords:
        for i in range(dados["DIMENSION"]):
            line = file.readline().split()

            dados["COORDS"].append([float(line[1]), float(line[2])])

    file.close()
    return dados