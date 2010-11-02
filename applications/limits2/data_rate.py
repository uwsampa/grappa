import re
import sys

p_brackets = re.compile("{([^}]+)}")
p_datarate = re.compile("data_rate:(\d+(\.\d+)?)")

size = sys.argv[1]
threads = sys.argv[2]
conc = sys.argv[3]
f = open("./ts_data2/LL_"+size+"_"+threads+"_"+conc+".out")

total = 0;
count = 0;

for line in f:
	m = re.match(p_brackets, line);
	if m is not None:
		m2 = re.search(p_datarate, m.group(0))
		total += float(m2.group(1))
		count += 1

print size+","+threads+","+conc+","+str(total/count)
			
		
