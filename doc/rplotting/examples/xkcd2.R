library(ggplot2)
library(extrafont)

### Already have read in fonts (see previous answer on how to do this)
loadfonts()

### Clegg and Pleb data
pleb.clegg <- read.csv("pleb and clegg.csv")
pleb.clegg$Date <- as.Date(pleb.clegg$Date, format="%d/%m/%Y")
pleb.clegg$xaxis <- -4

### XKCD theme
theme_xkcd <- theme(
  panel.background = element_rect(fill="white"),
  axis.ticks = element_line(colour=NA),
  panel.grid = element_line(colour="white"),
  axis.text.y = element_text(colour=NA),
  axis.text.x = element_text(colour="black"),
  text = element_text(size=16, family="Humor Sans")
)

### Plot the chart
p <- ggplot(data=pleb.clegg, aes(x=Date, y=Pleb))+
  geom_smooth(aes(y=Clegg), colour="gold", size=1, position="jitter", fill=NA)+
  geom_smooth(colour="white", size=3, position="jitter", fill=NA)+
  geom_smooth(colour="dark blue", size=1, position="jitter", fill=NA)+
  geom_text(data=pleb.clegg[10, ], family="Humor Sans", aes(x=Date), colour="gold", y=20, label="Searches for clegg")+
  geom_text(data=pleb.clegg[22, ], family="Humor Sans", aes(x=Date), colour="dark blue", y=4, label="Searches for pleb")+
  geom_line(aes(y=xaxis), position = position_jitter(h = 0.1), colour="black")+
  coord_cartesian(ylim=c(-5, 40))+
  labs(x="", y="", title="Pleb vs Clegg: Google Keyword Volumes")+
  theme_xkcd

ggsave("xkcd_cleggpleb.jpg", plot=p, width=8, height=5)
