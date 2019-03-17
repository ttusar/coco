#!/usr/bin/env Rscript
require(stringr) 
setwd("/media/thehedgeify/Data/svn/gbea/code-postprocessing/gbea")

dir = "/media/thehedgeify/Data/svn/gbea/code-postprocessing/gbea/output-diagonal-walks-mario"
outfile = "logs"

files <- list.files(path=dir, pattern = "(.)*.dat$",recursive=T) #switch to "\\.tdat" if required

for(f in files){
  df <- data.frame(evaluation=integer(), fitness=double(), dim=integer(), fun=integer(), inst=integer())
  print(f)
  dim <- 10
  fun <- as.numeric(str_extract(str_extract(f,"b[0-9]+"),"\\d+")) #extract function number
  inst <- as.numeric(str_extract(str_extract(f,"i[0-9]+"),"\\d+")) #extract instance number
  f = paste(dir,f, sep="/")
  res <- readLines(f) #read file
  eval = 0
  num=TRUE
  for(i in 1:length(res)){
    if(suppressWarnings(!is.na(as.numeric(res[i])))){
      num=TRUE
      df = rbind(df, c(eval, as.numeric(res[i]), dim, fun, inst))
    }else if(num){#so already not numeric
      eval = eval+1
      num=FALSE
    }
  }
  colnames(df) = c("evaluation", "fitness", "dim", "fun", "inst")
  save(df, file=paste("logs-post/",outfile, "_f",fun,"_i",inst,".RData", sep=""))
}

