#!/usr/bin/env Rscript
args = commandArgs(trailingOnly=TRUE)
if(length(args)!=2){
  stop("looking for 2 arguments, path to input folder and name of output file")
}

## or else just load stringr
require(stringr) 

dir = args[1]
name = args[2]
#dir = "/media/thehedgeify/Data/svn/gbea/code-postprocessing/gbea/rw-gan-mario-lhs-rw"
#name = "rw-gan-mario-lhs.RData"
print(dir)
print(name)

## get file names (required to be in main path of the experiments data folder)
files <- list.files(path=dir, pattern = "(.)*SAPEOcma.txt$",recursive=T) #switch to "\\.tdat" if required
df <- data.frame(dim=integer(), fun=integer(), inst=integer(), inc_r=integer(), corr_r=integer(), inc_sel=integer(), corr_sel=integer())
for(f in files){
  print(f)
  dim <- as.numeric(str_extract(str_extract(f,"_d[0-9]+"),"\\d+")) #extract dimension
  fun <- as.numeric(str_extract(str_extract(f,"_f[0-9]+"),"\\d+")) #extract function number
  inst <- as.numeric(str_extract(str_extract(f,"_i[0-9]+"),"\\d+")) #extract instance number
  f = paste(dir,f, sep="/")
  r_error <- read.table(f,header=F)[,18] #convert lines to table
  res = table(r_error)
  sel <- read.table(f,header=F)[,c(17,18)] #convert lines to table
  sel[,1] = sapply(sel[,1], as.character)
  sel[,1] = as.numeric(sapply(sel[,1], substr, start=1, stop=1))
  ind = sel[,1]<=5
  sel_error = table(sel[ind,2])
  data = c(dim, fun, inst, res[2], res[4], sel_error[2], sel_error[4])
  df = rbind(df,data)
}
colnames(df) = c("dim", "fun", "inst", "inc_r", "corr_r", "inc_sel", "corr_sel")

save(df, file=name)