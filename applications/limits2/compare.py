import re
import sys

p_brackets = re.compile("{([^}]+)}")

# experiement parameters
size = sys.argv[1]
threads = sys.argv[2]
conc = sys.argv[3]

# labels to extract
calc_label = sys.argv[4]
counter_label = sys.argv[5]

p_calc_label = re.compile(calc_label+":(\d+(\.\d+)?)")
p_counter_label = re.compile("(\d+)\s"+counter_label)

# open data file for reading
f = open("./cmp_data/LL_"+size+"_"+threads+"_"+conc+".out")

calc_total = 0;
calc_count = 0;

counter_total = 0;
counter_count = 0;


for line in f:
	m = re.match(p_brackets, line);
	if m is not None:
		m2 = re.search(p_calc_label, m.group(0))
		calc_total += float(m2.group(1))
		calc_count += 1
	
	n = re.match(p_counter_label, line)
	if n is not None:
		counter_total += int(n.group(1))
		counter_count += 1

#print calc_count,counter_count
assert(calc_count==counter_count)

calc_avg = str(calc_total/calc_count)
counter_avg = str(counter_total/counter_count)

print size+","+threads+","+conc+","+calc_avg+","+counter_avg
			
		
