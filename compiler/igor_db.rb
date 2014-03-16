require_relative_to_symlink '../util/igor_common.rb'

# cute trick for running inside an object
def with(obj, &blk); obj.instance_eval(&blk); end

Igor do
  database(adapter:'mysql', host:'n03.sampa', user:'grappa', database:'grappa')
end
