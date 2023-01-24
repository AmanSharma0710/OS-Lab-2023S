groupno = 34
files = []
for i in [1, 2, 4, 7, 9]:
    files.append("Assgn1_{}_{}.sh".format(i, groupno))
wc = 0
for file in files:
    with open(file) as f:
        content = f.read().replace('=', ' = ').replace('|', ' | ').replace(';', ' ; ').replace(',', ' , ').replace('>', ' > ').replace('<', ' < ').replace('(', ' ( ').replace(')', ' ) ').replace('[', ' [ ').replace(']', ' ] ').replace('{', ' { ').replace('}', ' } ')
        wc += len(content.split())
        print(file, len(content.split()))
print("Total: ", wc)
