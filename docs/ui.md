# UI documentation #

## Coordinate system ##

The UI uses Cartesian coordinates. (0, 0) is located at the lower-left corner of the screen.

## UI primitives ##

| name | keyword | description |
|------|---------|---|
| arc | \:arc | An intersection of a rectangle, and a circle or complement of a circle |
| rectangle | \:rect | A rectangle |
| text | \:text | Text |
| image | \:image | An image |

Primitives are specified using maps. All coordinates are integers. R, G, B and alpha values are in the 0-255 range. The type of primitive is specified using :type keyword. Example:

    {:type :arc
     :origin [0 440]
     :color :secondary
     :extents [20 40]
     :center [20 20]
     :radius 20}

### \:arc ###

Properties:

| name | value | description |
|---------|--------|------|
| :origin | \[x y\] | Coordinates of the lower-left corner of the rectangle |
| :extents | \[width height\] | Extents of the rectangle |
| :color | \[r g b\] or \[r g b alpha\] | Color of the rectangle |
| :center | \[x y\] | Coordinates of the center of the circle |
| :radius | radius | Radius of the circle |
| :inverted? | :true or nil (default) | Is the circle inverted? |

### :rect ###

Properties:

| name | value | description |
|---------|--------|------|
| :origin | \[x y\] | Coordinates of the lower-left corner of the rectangle |
| :extents | \[width height\] | Extents of the rectangle |
| :color | \[r g b\] or \[r g b alpha\] | Color of the rectangle |

### :text ###

| name | value | description |
|---------|--------|------|
| :origin | \[x y\] | Coordinates of the text (see *:text-anchor* and *:text-vanchor*) |
| :text-color | \[r g b\] or \[r g b alpha\] | Color of the rectangle |
| :font | :font-id | Font ID of the text |
| :text | string | The text |
| :text-anchor | :left (default), :center, or :right | Determines whether the x coordinate of *origin* specifies the left side, center, or right side of the text  |
| :text-vanchor | :bottom, :baseline (default), :center, :baseline-center, or :top | Determines whether the y coordinate of *origin* specifies the bottom, baseline, center, center excluding the area below the baseline, or top of the text |

### :image ###

| name | value | description |
|---------|--------|------|
| :origin | \[x y\] | Coordinates of the image (see *:anchor* and *:vanchor*) |
| :image | :image-id | Image ID |
| :anchor | :left (default), :center, or :right | Determines whether the x coordinate of *origin* specifies the left side, center, or right side of the image  |
| :vanchor | :bottom (default), :center, or :top | Determines whether the y coordinate of *origin* specifies the bottom, center, or top of the image |

## Primitive rules ##

- Font characters must not overlap.
- Arcs and rects must not overlap with one another.

Primitives are rendered in the following order:

- Images
- Arcs and rectangles
- Text

## UI elements ##

| name | keyword | description |
|------|---------|-------------|
| shape | :shape | An arbitrary shape composed of primitives |
| label | :label | A rectangular text label with a background color |
| button | :button | A rectangular or rounded button with text and a background color |
| image | :image | An image |
| top-left swept | :top-left-swept | A top-left corner filler |

A UI element is specified using a vector containing a keyword and a map, e.g.:

    [:image {:origin [127 240]
             :image :lock
             :anchor :center
             :vanchor :center}]

### :shape ###

| name | value | description |
|------|-------|-------------|
| :origin | \[x y\] | |
| :color | :color-id | |
| :primitives | sequential collection of primitives | |

### :label ###

| name | value | description |
|------|-------|-------------|
| :id | keyword | |
| :origin | \[x y\] | |
| :extents | \[width height\] | |
| :color | :color-id | |
| :font | :font-id | |
| :text | string | |
| :text-color | :color-id | |
| :text-align | :left (default), :center, or :right | |
| :text-valign | :bottom, :baseline (default), :center, :baseline-center, or :top | |
| :margin | positive integer or 0 (default) | |

### :button ###

| name | value | description |
|------|-------|-------------|
| :id | keyword | |
| :origin | \[x y\] | |
| :extents | \[width height\] | |
| :color | :color-id | |
| :font | :font-id | |
| :text | string | |
| :text-color | :color-id | |
| :text-align | :left (default), :center, or :right | |
| :text-valign | :bottom, :baseline (default), :center, :baseline-center, or :top | |
| :margin | positive integer or 0 (default) | |
| :button-style | :rect (default), or :rounded | |
| :on-click | fn: state &rarr; state, or nil (default) | |
| :on-touch-up | fn: state &rarr; state, or nil (default) | |
| :on-touch-down | fn: state &rarr; state, or nil (default) | |
| :on-touch-lost | fn: state &rarr; state, or nil (default) | |

### :image ###

| name | value | description |
|------|-------|-------------|
| :origin | \[x y\] | Coordinates of the image (see *:anchor* and *:vanchor*) |
| :image | :image-id | Image ID |
| :anchor | :left (default), :center, or :right | Determines whether the x coordinate of *origin* specifies the left side, center, or right side of the image  |
| :vanchor | :bottom (default), :center, or :top | Determines whether the y coordinate of *origin* specifies the bottom, center, or top of the image |

### :top-left-swept ###

| name | value | description |
|------|-------|-------------|
| :origin | \[x y\] | |
| :extents | \[width height\] | |
| :inner-radius | positive integer | |
| :color | :color-id | |

## UI components ##

    (ui/defc name [state & args] body)
