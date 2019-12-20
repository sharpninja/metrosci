# Convert Embedded.properties into C++ source code to include in Schott
count = 0
section = 1
with open("Embedded.cpp", "w") as o:
	o.write("const char *props" + str(section) + " = \n")
	section += 1
	with open("Embedded.properties") as f:
		for l in f.readlines():
			if l.endswith('\n'):
				l = l[:-1]
			l = l.replace('\\', '\\\\')
			l = l.replace('"', '\\"')
			l = '"' + l + '\\n"\n'
			o.write(l)
			count += len(l)
			# Visual C++ doesn't allow string literals to be larger than 64K
			if count > 60000:
				o.write(";\n")
				o.write("const char *props" + str(section) + " = \n")
				section += 1
				count = 0
