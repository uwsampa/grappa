my_theme <- theme(
  panel.background = element_rect(fill="white"),
  panel.border = element_rect(fill=NA, color="grey50"),
  panel.grid.major = element_line(color="grey80", size=0.3),
  panel.grid.minor = element_line(color="grey90", size=0.3),
  strip.background = element_rect(fill="grey90", color="grey50"),
  strip.background = element_rect(fill="grey80", color="grey50"),
  axis.ticks = element_line(colour="black"),
  panel.grid = element_line(colour="black"),
  axis.text.y = element_text(colour="black"),
  axis.text.x = element_text(colour="black"),
  text = element_text(size=14, family="Humor Sans")
)

prettify <- function(str) gsub('_',' ',gsub('([a-z])([a-z]+)',"\\U\\1\\E\\2",str,perl=TRUE))
