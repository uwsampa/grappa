library(ggplot2)
library(extrafont)
library(sqldf)
loadfonts()

# Download XKCD-style font
# download.file("http://simonsoftware.se/other/xkcd.ttf", dest="xkcd.ttf")

sosp <- function(query) {
  return(sqldf(query, dbname="sosp.db"))
}

### XKCD theme
theme_xkcd <- theme(
  panel.background = element_rect(fill="white"),
  axis.ticks = element_line(colour="black"),
  panel.grid = element_line(colour="black"),
  axis.text.y = element_text(colour="black"),
  axis.text.x = element_text(colour="black"),
  text = element_text(size=16, family="Humor Sans")
)

# Import the font and load fonts
# font_import(".")

# Load data, exported from Google Analytics
# site_data = read.csv("fake_pageviews.csv")
d <- sosp("select * from bfs")

# Make plot with ggplot
# p <- ggplot(d, aes(x=nnode, y=max_teps)) +
p <- qplot(data=d, x=nnode, y=max_teps) +
  geom_point() +
  geom_smooth(position="jitter", fill=NA, method="loess", size=2) +
  ggtitle("Grappa") +
  xlab("Nodes") + ylab("Max TEPS") +
  theme_xkcd

# Show plot
p
