# Copyright 2015 Sindre Ilebekk Johansen and Andreas Sløgedal Løvland

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

#     http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

library("ggplot2")
library("extrafont")

args <- commandArgs(trailingOnly = TRUE)

data <- read.csv(file="stdin")

#data$y = data$y + 1

c <- ggplot(data, aes(x, y))

max_y <- max(data$y)

jitter_factor <- 0.04
max_jitter <- 0.3

if (max_y >= max_jitter / jitter_factor) {
    jitter <- max_jitter;
} else {
    jitter <- max_y * jitter_factor;
}



plot <- c + stat_smooth(method="loess", span=0.40, level=0.999, se=FALSE, size=1.0, alplha=0.8) + geom_jitter(position=position_jitter(h=jitter, w=0), size=0.9, alpha=0.6)

plot <- plot + xlab('Throughput') + ylab('Incidents')
plot <- plot + theme_bw() + theme(text=element_text(family="CM Roman"))


if (max_y > 40) {
    by_y <- 10
} else if (max_y > 10) {
    by_y <- 2
} else {
    by_y <- 1
}

extra_y_minus <- 0.3

# Sorry, deadline....
if (grepl(".*symetric.*",args[1])) {
    extra_y_minus <- 10
}

plot_scaled <- plot + scale_x_continuous(breaks = round(seq(min(data$x), max(data$x), by = 1000),-3), limits=c(min(data$x)-0.3, max(data$x)+0.3))
plot_scaled <- plot_scaled + scale_y_continuous(breaks = round(seq(min(data$y), max(data$y), by = by_y),0), limits=c(min(data$y)-extra_y_minus, max(data$y)+0.3))

ggsave(filename=args[1], plot=plot_scaled, width=4.85, height=3.0, dpi=300, scale=1.2)

if (grepl(".*symetric.*",args[1])) {
    plot_scaled <- plot + scale_x_continuous(breaks = round(seq(min(data$x), max(data$x), by = 1000),-3), limits=c(min(data$x)-0.3, max(data$x)+0.3))
    plot_scaled <- plot_scaled + scale_y_continuous(breaks = round(seq(min(data$y), 10, by = 1),0), limits=c(min(data$y)-0.3, 10.0+0.3))

    ggsave(filename='pdf/symetric_zoom.pdf', plot=plot_scaled, width=4.85, height=3.0, dpi=300, scale=1.2)

}
