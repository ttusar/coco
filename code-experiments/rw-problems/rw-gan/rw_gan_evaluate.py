# This executes the GAN and evaluates it
# problem ids
# id = g + f*G + c*F*G
# where g is GAN-type (from which json)
# f is function
# c is whether concatenated or not
import torch
from torch.autograd import Variable

import sys
import os
import numpy
import pytorch.models.dcgan as dcgan
import glob

batchSize = 64

imageSize = 32
ngf = 64
ngpu = 1
n_extra_layers = 0

features = 13
budget = 5000

GROUND = 0
BREAK = 1
PASS = 2
QUESTIONC = 3
QUESTIONP = 4
COIN = 5
TUBE = 6
PLANT = 7
BILL = 8
GOOMBA = 9
GKOOPA = 10
RKOOPA = 11
SPINY = 12

#        tiles.put('X', 0); //solid
#        tiles.put('x', 1); //breakable
#        tiles.put('-', 2); //passable
#        tiles.put('q', 3); //question with coin
#        tiles.put('Q', 4); //question with power up
#        tiles.put('o', 5); //coin
#        tiles.put('t', 6); //tube
#        tiles.put('p', 7); //piranha plant tube
#        tiles.put('b', 8); //bullet bill
#        tiles.put('g', 9); //goomba
#        tiles.put('k', 10); //green koopas + paratroopas
#        tiles.put('r', 11); //red koopas + paratroopas
#        tiles.put('s', 12); //spiny + winged spiny

batchSize = 1

#################################################################################
# Utils

def exist_gap(im):
    # gap exists if not 10 (coin), not 2 (passable)
    width = numpy.shape(im)[1]
    height = numpy.shape(im)[0]
    gaps = numpy.zeros(width)
    for i in range(0, width):
        imc = im[:, i]
        unique, counts = numpy.unique(imc, return_counts=True)
        dist = dict(zip(unique, counts))
        if dist.get(COIN, 0) + dist.get(PASS, 0) == height:  # all tiles in column passable
            gaps[i] = 1
    return gaps


def count_gaps(im):
    gaps = exist_gap(im)
    return sum(gaps)


def gap_lengths(im):
    gaps = exist_gap(im)
    gaps = "".join([str(int(x)) for x in gaps])
    return map(len, gaps.split('0'))


def max_gap(im):
    return max(gap_lengths(im))


def count_tile_type(im, tile):
    num_tiles = (len(im[im == tile]))
    return num_tiles


def gan_maximise_tile_type(x, nz, tile):
    return -count_tile_type(x, nz, tile)


def gan_target_tile_type(x, nz, tile, target):
    return abs(target - count_tile_type(x, nz, tile))


def gan_target_tile_type_frac(im, tile, target_frac):
    total_tiles = float(numpy.shape(im)[0] * numpy.shape(im)[1])
    return abs(target_frac - (count_tile_type(im, tile) / total_tiles))



def tilePositionSummaryStats(im, tiles):
    coords = numpy.where(im==tiles[0])
    x_coords = coords[1]
    y_coords = coords[0]
    for tile in tiles[1:]:
        tmp = numpy.where(im==tile)
        x_coords = numpy.append(x_coords, tmp[1])
        y_coords = numpy.append(y_coords, tmp[0])
    return numpy.mean(x_coords), numpy.std(x_coords), numpy.mean(y_coords), numpy.std(y_coords)

################################################################################################
# Fitness Functions

# Estimates the leniency of the level
# Value range?
# minimise
def leniency(x, netG, dim):
    im = translateLatentVector(x, netG, dim)
    unique, counts = numpy.unique(im, return_counts=True)
    dist = dict(zip(unique, counts))
    val = 0
    val += dist.get(QUESTIONP, 0) * 1
    val += dist.get(PLANT, 0) * (-1)
    val += dist.get(BILL, 0) * (-1)
    val += dist.get(GOOMBA, 0) * (-1)
    val += dist.get(GKOOPA, 0) * (-1)
    val += dist.get(RKOOPA, 0) * (-1)
    val += dist.get(SPINY, 0) * (-1)
    val += count_gaps(im) * (-0.5)
    t = numpy.array(gap_lengths(im))
    if count_gaps(im) > 0:
        val -= numpy.mean(t[t != 0])
    outputResult(val, 1)

# Percentage of stackable items
# Value range 0-1
# maximise
def density(x, netG, dim):
    im = translateLatentVector(x, netG, dim)
    unique, counts = numpy.unique(im, return_counts=True)
    dist = dict(zip(unique, counts))
    width = numpy.shape(im)[1]
    height = numpy.shape(im)[0]
    val = 0
    # Tiles you can stand on
    val += dist.get(GROUND, 0)
    val += dist.get(BREAK, 0)
    val = float(val) / (width * height)
    outputResult(-val, 1)


# Estimates how much of the space can be reached by computing how many of the tiles can be stood upon
# Value Range 0-1
# maximise
def negativeSpace(x, netG, dim):
    im = translateLatentVector(x, netG, dim)
    unique, counts = numpy.unique(im, return_counts=True)
    dist = dict(zip(unique, counts))
    width = numpy.shape(im)[1]
    height = numpy.shape(im)[0]
    val = 0
    # Tiles you can stand on
    val += dist.get(GROUND, 0)
    val += dist.get(BREAK, 0)
    val += dist.get(QUESTIONC, 0)
    val += dist.get(QUESTIONP, 0)
    val += dist.get(TUBE, 0) * 2 # Because only one tile, but width of 2
    val += dist.get(PLANT, 0) * 2 # Because only one tile, but width of 2
    val += dist.get(BILL, 0)
    val = float(val) / (width * height)
    outputResult(-val, 1)


# Frequency of pretty tiles, i.e. non-standard.
# Value Range 0-1
# maximise
def decorationFrequency(x, netG, dim):
    im = translateLatentVector(x, netG, dim)
    unique, counts = numpy.unique(im, return_counts=True)
    dist = dict(zip(unique, counts))
    width = numpy.shape(im)[1]
    height = numpy.shape(im)[0]
    val = 0
    # Pretty tiles according to paper
    val += dist.get(BREAK, 0)
    val += dist.get(QUESTIONC, 0)
    val += dist.get(QUESTIONP, 0)
    val += dist.get(COIN, 0)
    val += dist.get(TUBE, 0)
    val += dist.get(PLANT, 0)
    val += dist.get(BILL, 0)
    val += dist.get(GOOMBA, 0)
    val += dist.get(GKOOPA, 0)
    val += dist.get(RKOOPA, 0)
    val += dist.get(SPINY, 0)
    val = float(val) / (width * height)
    outputResult(-val, 1)

# gets vertical distribution of tiles you can stand on
# Value range ?
# maximise
def positionDistribution(x, netG, dim):
    im = translateLatentVector(x, netG, dim)
    xm, xs, ym, ys = tilePositionSummaryStats(im, [GROUND, BREAK, QUESTIONP, QUESTIONC, TUBE, PLANT, BILL])
    outputResult(-ys, 1)


# get horizontal distribution of enemies
# Value range ?
# maximise
def enemyDistribution(x, netG, dim):
    im = translateLatentVector(x, netG, dim)
    xm, xs, ym, ys = tilePositionSummaryStats(im, [PLANT, BILL, GOOMBA, GKOOPA, RKOOPA, SPINY])
    outputResult(-xs, 1)


def translateLatentVector(x, netG, dim):
    generator = dcgan.DCGAN_G(imageSize, dim, features, ngf, ngpu, n_extra_layers)
    generator.load_state_dict(torch.load(netG, map_location=lambda storage, loc: storage))
    inp = numpy.array_split(x, len(x) / dim)
    final = None
    for x in inp:
        latent_vector = torch.FloatTensor(x).view(batchSize, dim, 1, 1)
        levels = generator(Variable(latent_vector, volatile=True))
        levels.data = levels.data[:, :, :14, :28]
        im = levels.data.cpu().numpy()
        im = numpy.argmax(im, axis=1)
        if final is None:
            final = im[0]
        else:
            final = numpy.column_stack([final, im[0]])
    print(final)
    return final


def outputResult(result, d=1):
    print(result)
    with open('objectives.txt', 'w') as f:
        f.write('{}\n'.format(d))
        f.write('{}\n'.format(result))
        f.close()


def progressSimAStar(x, netG, dim):
    os.system('java -jar marioaiDagstuhl.jar "' + str(content[1:]) + '" ' + netG + ' ' + str(dim) + ' ' + str(0) + ' ' + str(0))


def basicFitnessSimAStar(x, netG, dim):
    os.system('java -jar marioaiDagstuhl.jar "' + str(content[1:]) + '" ' + netG + ' ' + str(dim) + ' ' + str(1) + ' ' + str(0))


def jumpFractionSimAStar(x, netG, dim):
    os.system('java -jar marioaiDagstuhl.jar "' + str(content[1:]) + '" ' + netG + ' ' + str(dim) + ' ' + str(2) + ' ' + str(0))


def totalActionsSimAStar(x, netG, dim):
    os.system('java -jar marioaiDagstuhl.jar "' + str(content[1:]) + '" ' + netG + ' ' + str(dim) + ' ' + str(3) + ' ' + str(0))

def progressSimREALM(x, netG, dim):
    os.system('java -jar marioaiDagstuhl.jar "' + str(content[1:]) + '" ' + netG + ' ' + str(dim) + ' ' + str(0) + ' ' + str(1))

def basicFitnessSimREALM(x, netG, dim):
    os.system('java -jar marioaiDagstuhl.jar "' + str(content[1:]) + '" ' + netG + ' ' + str(dim) + ' ' + str(1) + ' ' + str(1))

def jumpFractionSimREALM(x, netG, dim):
    os.system('java -jar marioaiDagstuhl.jar "' + str(content[1:]) + '" ' + netG + ' ' + str(dim) + ' ' + str(2) + ' ' + str(1))

def totalActionsSimREALM(x, netG, dim):
    os.system('java -jar marioaiDagstuhl.jar "' + str(content[1:]) + '" ' + netG + ' ' + str(dim) + ' ' + str(3) + ' ' + str(1))


# expecting variables <prob> <dim> <instance>
if __name__ == '__main__':
    _, dim, problem, inst = sys.argv
    problem = int(problem)-1
    dim = int(dim)
    inst = int(inst) - 1
    # TODO value ranges and how to set up

    available_dims = [10, 20, 30, 40]
    available_instances = [5641, 3854, 8370, 494, 1944, 9249, 2517, 2531, 5453, 2982, 670, 56, 6881, 1930, 5812]
    available_jsons = ["underground"]#"'["overworld"]#, "underground", "overworlds"]  # G
    available_fit = [enemyDistribution, positionDistribution, decorationFrequency, negativeSpace, leniency, density,
                     progressSimAStar, basicFitnessSimAStar, jumpFractionSimAStar, totalActionsSimAStar,
                     progressSimREALM, basicFitnessSimREALM, jumpFractionSimREALM, totalActionsSimREALM]  # F

    if dim not in available_dims:  # check Dimension available
        raise ValueError("asked for dimension '{}', but is not available".format(dim))
    if inst < 0 | inst >= available_instances.count():
        raise ValueError("asked for instance '{}', but is not available".format(inst))

    # Read the variables
    with open('variables.txt') as file:
        content = file.readlines()
        content = [float(line.rstrip('\n')) for line in content]
        num_variables = int(content[0])
        if num_variables != dim:  # check appropriate number of variables there
            raise ValueError("num_variables should be '{}', but is '{}'"
                             "".format(dim, num_variables))
        file.close()

    # Decode Problem id
    c = int(problem / (len(available_jsons) * len(available_fit)))
    tmp = problem % (len(available_jsons) * len(available_fit))
    f = int(tmp / len(available_jsons))
    g = tmp % len(available_jsons)

    if c == 1:
        dim = 10

    print([c, f, g])

    # check variables in range
    inp = numpy.array(content[1:])
    if numpy.any(inp > 1) or numpy.any(inp < -1):  # input out of range
        with open('objectives.txt', 'w') as file:  # write out NaN result
            file.write('{}\n'.format(0))
            file.close()
    else:
        # find correct file with highest epoch
        pattern = "GAN/{}-{}-{}/netG_epoch_*_{}.pth".format(available_jsons[g], dim, budget, available_instances[inst])
        files = glob.glob(pattern)
        epochs = [int(str.split(file, "_")[2]) for file in files]
        netG = "GAN/{}-{}-{}/netG_epoch_{}_{}.pth".format(available_jsons[g], dim, budget, max(epochs),
                                                          available_instances[inst])

        fun = available_fit[f]
        fun(content[1:], netG, dim)
