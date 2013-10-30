#
# Compliments of @igrigorik
# @see: https://gist.github.com/igrigorik/a491cc732b5d4627e193
#
# $ gem install garb
# $ ruby report.rb login@gmail.com pass UA-XXXXX-X
#
require 'garb'
require 'json'

# number of top pages to get data on
LIMIT_PAGES = 25
LIMIT_DESTINATIONS = 10

user, pass, property = ARGV

class TopPages
  extend Garb::Model
  metrics :pageviews, :visits, :exitrate
  dimensions :page_path
end

class Destinations
  extend Garb::Model
  metrics :pageviews
  dimensions :page_path
end

Garb::Session.login(user, pass)
profile = Garb::Management::Profile.all.detect {|p| p.web_property_id == property}

result = []

top = TopPages.results(profile, :limit => LIMIT_PAGES, :sort => :pageviews.desc)
top.each do |page|
  destinations = Destinations.results(profile, {
    :filters => {:previouspagepath.eql => page.page_path},
    :limit => LIMIT_DESTINATIONS, :sort => :pageviews.desc
  }).reject {|d| d.page_path == page.page_path }

  total = destinations.reduce(0) {|t,v| t+=v.pageviews.to_i}

  destinations.each do |dest|
    prob = (dest.pageviews.to_f / page.pageviews.to_f)
    result << [page.page_path, dest.page_path, prob]
  end
end

puts JSON result