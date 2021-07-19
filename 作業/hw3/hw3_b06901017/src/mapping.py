import sys

dic={}

infile=sys.argv[1]
f=open(infile,'r',encoding='big5hkscs')
for line in f:
    s=line.split(' ')
    chinese=s[0]
    zhuyin=s[1].split('/')
    dic[chinese]=[chinese]
    for char in zhuyin:
        if char[0] in dic:
            dic[char[0]].append(chinese)
        else:
            dic[char[0]]=[chinese]
f.close()

outfile=sys.argv[2]
out=open(outfile,'w',encoding='big5hkscs')
for key in sorted(dic.keys()):
    out.write(key+' '+' '.join(dic[key])+'\n')
out.close()
