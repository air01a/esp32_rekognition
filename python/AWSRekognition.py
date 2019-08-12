import time
import cv2
import json
import boto3
import io
from PIL import Image


class AWSRekognition:

	# Socket init
	def __init__(self):
		self.aws_client = boto3.client("rekognition")

	def labelDetection(self,frame):
		ret=[]
		response = self.aws_client.detect_labels(Image={'Bytes':frame},MaxLabels=20)
		for label in response['Labels']:
			cat=label['Name']
			for instance in label['Instances']:
				w=instance['BoundingBox']['Width']*800
				h=instance['BoundingBox']['Height']*600
				x=instance['BoundingBox']['Left']*800+w/2
				y=instance['BoundingBox']['Top']*600+h/2
				prob=instance['Confidence']
				ret.append((cat,prob,x,y,w,h,'awsreko'))
		return ret


