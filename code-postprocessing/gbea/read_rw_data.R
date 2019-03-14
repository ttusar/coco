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
files <- list.files(path=dir, pattern = "(.)*_rw.txt$",recursive=T) #switch to "\\.tdat" if required

df <- data.frame(evaluation=integer(), fitness=double(), loc=list(), dim=integer(), fun=integer(), inst=integer())
for(f in files){
  print(f)
  dim <- as.numeric(str_extract(str_extract(f,"_d[0-9]+"),"\\d+")) #extract dimension
  fun <- as.numeric(str_extract(str_extract(f,"_f[0-9]+"),"\\d+")) #extract function number
  inst <- as.numeric(str_extract(str_extract(f,"_i[0-9]+"),"\\d+")) #extract instance number
  f = paste(dir,f, sep="/")
  res <- readLines(f) #read file
  readdf <- read.table(textConnection(res),header=F, fill=TRUE) #convert lines to table
  readdf = na.omit(readdf)
  loc = readdf[,3:(2+dim)] #x-values
  loc = apply(loc, 1, list)
  readdf <- readdf[,c(1,2,ncol(readdf))]
  colnames(readdf) = c("evaluation", "fitness", "time")
  readdf$loc = loc
  readdf$dim = dim
  readdf$fun = fun
  readdf$inst = inst
  df <- rbind(df,readdf) #append result
}
save(df,file=name)

find_runs = function(){
  idx_start = which(df$fun==min(df$fun) & df$ins==min(df$inst) & df$evaluation==1)
  runs = numeric(0)
  for(i in 1:(length(idx_start)-1)){
    runs= c(runs, rep((i-1), idx_start[i+1]-idx_start[i]))
  }
  runs = c(runs, rep((length(idx_start)-1), nrow(df)-idx_start[length(idx_start)]+1))
  return(runs)
}
