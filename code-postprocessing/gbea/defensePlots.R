exp =c(32, 33, 34, 35, 40)
setwd("/media/vv/DATA/svn/gbea/code-postprocessing/gbea")
data = data.frame(evaluation=integer(), fitness=double(), loc=list(), dim=integer(), fun=integer(), inst=integer(), exp=integer())
for(x in exp){
  load(paste("data", x, ".RData", sep=""))
  df$exp = x
  data = rbind(data,df)
}

data$dim = as.factor(data$dim)
data$fun = as.factor(data$fun)
data$inst = as.factor(data$inst)
data$exp = as.factor(data$exp)
cols = rainbow(length(exp))
exp2 = exp
exp2 = c(32, 33, 40)

data2 = data.frame(evaluation=integer(), fitness=double(), loc=list(), dim=integer(), fun=integer(), inst=integer(), exp=integer())
for(f in unique(data$fun[inds])){
  plot(NA, xlim=c(0,1000), ylim=c(0,1), main=f, xlab="evaluation", ylab="best fitness")
  legend("topright", legend=exp, col=cols, lty=1)
  for(x in exp2){
    inds = data$exp==x
    for(i in unique(data$inst[inds])){
      dt = data[data$fun==f & inds & data$inst == i,]
      min = 2000
      idx = logical(nrow(dt))
      idx[1] = TRUE
      for(k in 2:nrow(dt)){
        if(!is.na(dt$fitness[k]) & dt$fitness[k]<min){
          idx[k]=TRUE
          min=dt$fitness[k]
        }else{
          idx[k]=FALSE
        }
      }
      dt = dt[idx,]
      dt = rbind(dt, dt[nrow(dt),])
      dt$evaluation[nrow(dt)] =5000
      data2 = rbind(data2, dt)
      lines(c(dt$evaluation,1000), c(dt$fitness, min(dt$fitness)), col=cols[which(exp==x)], type="s")
    }
  }
}

library(plotly)
library(plyr)

data2$exp = revalue(data2$exp, c("32"="CMA-ES", "33"="SAPEO", "34" ="SAPEO - density", "35" = "SAPEO - density + loc", "40"="Random Search"))

p <- plot_ly(data2, x = ~evaluation, y = ~fitness, type = 'scatter', mode = 'lines', line = list(shape = "hv"), color = ~exp,
             linetype=~inst) %>%
  #layout(title = '25 airTime A* overworld I',
  layout(title = '7 decorationFrequency overworld I',
         xaxis = list(title = 'evaluation'),
         yaxis = list (title = 'best fitness'))

htmlwidgets::saveWidget(as_widget(p), "index.html")
