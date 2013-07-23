pagerank <- function(M, d, v_quadratic_error) {
  N <- dim(M)[1]
  v <- runif(N, min=0, max=1)
  #v <- c(0.840188,0.394383,0.783099,0.79844)
  v <- v / norm(v, "2")
  print(v)
  last_v <- vector(mode="numeric", length=N) - 1000
  M.hat <- (d * M) + (((1-d) / N) * matrix(1,N,N))
  print(M.hat)
  iterations <- 0
  norms <- list()
  
  while( norm(v-last_v, "2") > v_quadratic_error) {
    print(sprintf("norm: %f", norm(v-last_v, "2")))
    norms <- append(norms, norm(v-last_v,"2"))
    iterations <- iterations + 1
    last_v <- v
    v <- M.hat %*% v
    v <- v/ norm(v, "2")
  }
  
  print(iterations)
  return(list(v,norms))
}

pagerank_iterative <- function(M, d, v_quadratic_error) {
  N <- dim(M)[1]
  v <- runif(N, min=0, max=1)
  #v <- c(0.840188,0.394383,0.783099,0.79844)
  v <- v / norm(v, "2")
  print(v)
  last_v <- vector(mode="numeric", length=N) - 1000
  dM <- d*M
  print(dM)
  iterations <- 0
  norms <- list()
  
  while( norm(v-last_v, "2") > v_quadratic_error) {
   # print(sprintf("norm: %f", norm(v-last_v, "2")))
    norms <- append(norms, norm(v-last_v,"2"))
    iterations <- iterations + 1
    last_v <- v
    v <- (dM %*% v) + (1-d)/N
    v <- v/ norm(v, "2")
  }
  #print(sprintf("norm: %f", norm(v-last_v, "2")))
  norms <- append(norms, norm(v-last_v,"2"))
  return (list(v, norms))
}

M <- matrix(c(1,0,0,.25,
         1,1,1,.25,
         0,0,1,.25,
         0,0,1,0), 4, 4)
res <- pagerank(M, 0.80, 0.001)
ranks <- res[[1]]
norms <- res[[2]]


M <- t(matrix(c(0,   0, 0.2, 0.2,
                0,   0, 0.2,   0,
                0.2, 0, 0.2,   0.2,
                0.2, 0, 0.2, 0), 4, 4))
M <- t(matrix(c(0,1,1,0,1,1,
                1,0,1,1,0,1,
                0,0,0,0,0,1,
                0,0,0,0,1,0,
                0,0,0,1,0,0,
                0,1,1,1,1,0), 6,6))
star_special <- t(matrix(c(0,0,1,1,1,1,
                1,0,0,0,0,0,
                0,0,0,0,0,0,
                0,0,0,0,0,0,
                0,0,0,0,0,0,
                0,0,0,0,0,0), 6,6,))
circle_star <- t(matrix(c(0,0,1,1,1,1,
                          1,0,0,0,0,0,
                          0,0,0,0,0,1,
                          0,0,0,0,1,0,
                          0,0,0,1,0,0,
                          0,0,1,0,0,0), 6,6,))
hay_bunch <- t(matrix(c(0,0,1,1,1,1,
                        1,0,0,0,0,0,
                        0,1,0,0,0,0,
                        0,1,0,0,0,0,
                        0,1,0,0,0,0,
                        0,1,0,0,0,0), 6,6))
chain <- t(matrix(c(0,1,0,0,0,0,
                    0,0,1,0,0,0,
                    0,0,0,1,0,0,
                    0,0,0,0,1,0,
                    0,0,0,0,0,1,
                    0,0,0,0,0,0),6,6))
res <- pagerank(M, 0.80, 0.01)
resi <- pagerank_iterative(M, 0.80, 0.001)


