#!/usr/bin/env Rscript
setwd("/media/thehedgeify/Data/svn/gbea/code-postprocessing/gbea")

dir = "/media/thehedgeify/Data/svn/gbea/code-postprocessing/gbea/logs-post"
outfile = "agg-logs"

files <- list.files(path=dir, pattern = "(.)*.RData$",recursive=T) #switch to "\\.tdat" if required


for(f in files){
  agg_df <- data.frame(fitness=double(), sd=double(), max_diff=double(), dim=integer(), fun=integer(), inst=integer())
  
  print(f)
  f = paste(dir,f, sep="/")
  load(f)
  
  dim <- df$dim[1]
  fun <- df$fun[1]
  inst <- df$inst[1]
  
  for(i in 1:max(df$evaluation)){
    data = df[df$evaluation==i,]
    agg_df = rbind(agg_df, c(mean(data$fitness), sd(data$fitness), max(data$fitness)-min(data$fitness), dim, fun, inst))
  }
  colnames(agg_df) = c("fitness", "sd", "max_diff", "dim", "fun", "inst")
  save(agg_df, file=paste("agg-logs-post/",outfile, "_f",fun,"_i",inst,".RData", sep=""))
}