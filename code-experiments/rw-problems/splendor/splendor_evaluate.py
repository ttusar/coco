# This executes Splendor Jar
import sys
import os
import numpy



#def outputResult(result, d=1, file_name="objectives.txt"):
    #print(result)
#    with open(file_name, 'w') as f:
#        f.write('{}\n'.format(d))
#        f.write('{}\n'.format(result))


java_options = "-Djava.awt.headless=true "


def callJar(x, fid, inst, dim, file_name):
    #print('java ' + java_options + '-jar splendor.jar ' +  "0 " +
     #         str(fid) + ' ' + str(inst) + ' ' + file_name + ' ' + str(dim) + ' ' + str(content[1:]))
    os.system('java ' + java_options + '-jar splendor.jar ' +  "0 " +
              str(fid) + ' ' + str(inst) + ' ' + file_name + ' ' + str(dim) + ' ' + str(content[1:]))## + ' > /dev/null')


#expecting variables <obj> <dim> <fun> <inst>
if __name__ == '__main__':
    _, obj, dim, problem, inst = sys.argv
    problem = int(problem)-1
    dim = int(dim)
    inst = int(inst) - 1
    obj = int(obj)
    # TODO value ranges and how to set up

  #1: sequenceLength, integer, 1-(200/number of players)
  #2: evals, integer, 0-budgetPerCall (1000)
  #3: flipatlestonevalue -> binary
  #4: useShiftBuffer -> binary*/

    available_dims = [2, 3, 4]
    available_instances = range(0,15)

    if obj != 1:
        raise ValueError("currently only 1 objective")
    if dim not in available_dims:  # check Dimension available
        raise ValueError("asked for dimension '{}', but is not available".format(dim))
    if inst < 0 | inst >= available_instances.count():
        raise ValueError("asked for instance '{}', but is not available".format(inst))

    # Read the variables
    file_name = "variables_o{:d}_f{:02d}_i{:02d}_d{:02d}.txt".format(obj, problem+1, inst+1, dim)
    with open(file_name) as file:
        content = file.readlines()
        content = [float(line.rstrip('\n')) for line in content]
        num_variables = int(content[0])
        if num_variables != dim:  # check appropriate number of variables there
            raise ValueError("num_variables should be '{}', but is '{}'"
                             "".format(dim, num_variables))

    if dim==2:
       #TODo add default variables, also for dim 3

    file_name = "objectives_o{:d}_f{:02d}_i{:02d}_d{:02d}.txt".format(obj, problem+1, inst+1, dim)
    callJar(content[1:], problem, inst, dim, file_name)
