class Book < ActiveRecord::Base
  acts_as_taggable_on :tags
end
