#!/usr/bin/env Rscript
setwd("/media/thehedgeify/Data/svn/gbea/code-postprocessing/gbea")

dir = "/media/thehedgeify/Data/svn/gbea/code-postprocessing/gbea/agg-logs-post"

files <- list.files(path=dir, pattern = "(.)*.RData$",recursive=T) #switch to "\\.tdat" if required
pdf("rough-noise-plots.pdf")

df <- data.frame(sd_min=double(),
                 sd_1st=double(),
                 sd_med=double(),
                 sd_mean=double(),
                 sd_3rd=double(),
                 sd_max=double(),
                 diff_min=double(),
                 diff_1st=double(),
                 diff_med=double(),
                 diff_mean=double(),
                 diff_3rd=double(),
                 diff_max=double(),
                 dim=integer(), fun=integer(), inst=integer())

for(f in files){

  print(f)
  f = paste(dir,f, sep="/")
  load(f)
  
  dim <- agg_df$dim[1]
  fun <- agg_df$fun[1]
  inst <- agg_df$inst[1]
  
  df = rbind(df, c(summary(agg_df$sd), summary(agg_df$max_diff), dim, fun, inst))

  plot(rep(agg_df$fitness,2), c(agg_df$sd, agg_df$max_diff), col=c(rep("orange", nrow(agg_df)),rep("brown", nrow(agg_df))),
       main=paste("d",dim,"f",fun,"i",inst), xlab="fitness", ylab="sd or max_diff")
}
dev.off()


colnames(df) = c("sd_min", "sd_1st","sd_med","sd_mean", "sd_3rd", "sd_max",
                 "diff_min","diff_1st", "diff_med", "diff_mean", "diff_3rd", "diff_max", "dim", "fun", "inst")
save(df, file="agg-agg-logs.RData")
